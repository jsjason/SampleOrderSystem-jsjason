#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../Controller/OrderController.h"

static std::time_t parseTestTime(const std::string& s) {
    std::tm tm = {};
    sscanf_s(s.c_str(), "%d-%d-%d %d:%d:%d",
             &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
             &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

class OrderApproval : public ::testing::Test {
protected:
    SampleRepository sampleRepo{""};
    OrderRepository  orderRepo{""};
    ProductionQueue  prodQueue{""};

    void addSample(const std::string& id, int stock, double yield = 0.92) {
        sampleRepo.add(Sample{id, "테스트 시료", 0.5, yield, stock});
    }

    std::string placeOrder(const std::string& sampleId, int qty) {
        return orderRepo.add(sampleId, "테스트 고객", qty).orderNumber;
    }
};

// -----------------------------------------------------------------------
// 주문 승인 - 재고 충분
// -----------------------------------------------------------------------

TEST_F(OrderApproval, 재고가_충분하면_승인시_CONFIRMED로_전환된다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);
    auto result = applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(result.newStatus, OrderStatus::CONFIRMED);
    EXPECT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::CONFIRMED);
}

TEST_F(OrderApproval, 재고가_충분하면_승인시_재고가_주문량만큼_차감된다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);
    applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(sampleRepo.findById("S-001")->stock, 400);
}

TEST_F(OrderApproval, 재고가_충분하면_승인시_생산_큐에_등록되지_않는다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);
    applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_TRUE(prodQueue.empty());
}

// -----------------------------------------------------------------------
// 주문 승인 - 재고 부족
// -----------------------------------------------------------------------

TEST_F(OrderApproval, 재고가_부족하면_승인시_PRODUCING으로_전환된다) {
    addSample("S-001", 50);
    auto num = placeOrder("S-001", 200);
    auto result = applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(result.newStatus, OrderStatus::PRODUCING);
    EXPECT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::PRODUCING);
}

TEST_F(OrderApproval, 재고가_부족하면_승인시_생산_큐에_자동_등록된다) {
    addSample("S-001", 50);
    auto num = placeOrder("S-001", 200);
    applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_FALSE(prodQueue.empty());
}

TEST_F(OrderApproval, 재고가_부족하면_부족분만큼_생산_작업이_생성된다) {
    addSample("S-001", 380, 0.92);
    auto num = placeOrder("S-001", 500);
    applyApproval(num, sampleRepo, orderRepo, prodQueue);
    int shortage = 500 - 380;  // 120
    int expected = static_cast<int>(std::ceil(shortage / (0.92 * 0.9)));
    ASSERT_TRUE(prodQueue.front().has_value());
    EXPECT_EQ(prodQueue.front()->actualQuantity, expected);
}

TEST_F(OrderApproval, 재고가_0이면_주문량_전체가_생산_대상이다) {
    addSample("S-001", 0, 0.92);
    auto num = placeOrder("S-001", 100);
    auto result = applyApproval(num, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(result.shortage, 100);
    int expected = static_cast<int>(std::ceil(100.0 / (0.92 * 0.9)));
    ASSERT_TRUE(prodQueue.front().has_value());
    EXPECT_EQ(prodQueue.front()->actualQuantity, expected);
}

// -----------------------------------------------------------------------
// 주문 거절
// -----------------------------------------------------------------------

TEST_F(OrderApproval, 거절하면_REJECTED로_전환된다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);
    EXPECT_TRUE(applyRejection(num, orderRepo));
    EXPECT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::REJECTED);
}

TEST_F(OrderApproval, 거절해도_재고는_변경되지_않는다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);
    applyRejection(num, orderRepo);
    EXPECT_EQ(sampleRepo.findById("S-001")->stock, 500);
}

// -----------------------------------------------------------------------
// 생산 완료 처리 (Phase 3b에서 구현)
// -----------------------------------------------------------------------

TEST_F(OrderApproval, 생산_완료시_PRODUCING에서_CONFIRMED로_전환된다) {
    addSample("S-001", 0);
    auto num = placeOrder("S-001", 5);
    applyApproval(num, sampleRepo, orderRepo, prodQueue);

    auto job    = prodQueue.front().value();
    auto start  = parseTestTime(job.startedAt);
    auto future = start + static_cast<std::time_t>(job.totalDuration) + 1;
    prodQueue.processCompleted(sampleRepo, orderRepo, future);

    EXPECT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::CONFIRMED);
}

TEST_F(OrderApproval, 생산_완료시_시료_재고가_실생산량만큼_증가한다) {
    addSample("S-001", 0);
    auto num    = placeOrder("S-001", 5);
    auto result = applyApproval(num, sampleRepo, orderRepo, prodQueue);

    auto job    = prodQueue.front().value();
    auto start  = parseTestTime(job.startedAt);
    auto future = start + static_cast<std::time_t>(job.totalDuration) + 1;
    prodQueue.processCompleted(sampleRepo, orderRepo, future);

    // addStock(actualQty) 후 deductStock(5) → 최종 재고 = actualQty - 5
    EXPECT_EQ(sampleRepo.findById("S-001")->stock, result.actualQuantity - 5);
}

TEST_F(OrderApproval, 생산_완료시_재고에서_주문량만큼_차감된다) {
    addSample("S-001", 0);
    auto num    = placeOrder("S-001", 5);
    auto result = applyApproval(num, sampleRepo, orderRepo, prodQueue);
    int  actQty = result.actualQuantity;  // > 5 이므로 최종 재고 > 0

    auto job    = prodQueue.front().value();
    auto start  = parseTestTime(job.startedAt);
    auto future = start + static_cast<std::time_t>(job.totalDuration) + 1;
    prodQueue.processCompleted(sampleRepo, orderRepo, future);

    EXPECT_LT(sampleRepo.findById("S-001")->stock, actQty);
}

// -----------------------------------------------------------------------
// 생산 중 주문의 재고 선점 고려
// -----------------------------------------------------------------------

TEST_F(OrderApproval, 생산중인_주문이_있으면_해당_수량을_제외한_가용재고로_판단한다) {
    addSample("S-001", 3);
    // 주문 A: stock(3) < qty(10) → PRODUCING, A 완료 시 10 차감 예정
    auto numA = placeOrder("S-001", 10);
    applyApproval(numA, sampleRepo, orderRepo, prodQueue);

    // 생산이 진행되어 stock = 3 + 7 = 10 (addStock으로 시뮬레이션)
    sampleRepo.addStock("S-001", 7);

    // 수정 전: stock(10) >= qty(5) → CONFIRMED (잘못된 판단 — 10이 A에 선점됨)
    // 수정 후: 가용재고 = 10 - 10 = 0 < 5 → PRODUCING (올바른 판단)
    auto numB = placeOrder("S-001", 5);
    auto result = applyApproval(numB, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(result.newStatus, OrderStatus::PRODUCING);
}

TEST_F(OrderApproval, 생산중_주문_차감_후_가용재고가_충분하면_새_주문은_CONFIRMED이다) {
    addSample("S-001", 3);
    // 주문 A: stock(3) < qty(10) → PRODUCING, A 완료 시 10 차감 예정
    auto numA = placeOrder("S-001", 10);
    applyApproval(numA, sampleRepo, orderRepo, prodQueue);

    // 생산이 진행되어 stock = 3 + 20 = 23
    sampleRepo.addStock("S-001", 20);

    // 가용재고 = 23 - 10 = 13 >= 5 → CONFIRMED
    auto numB = placeOrder("S-001", 5);
    auto result = applyApproval(numB, sampleRepo, orderRepo, prodQueue);
    EXPECT_EQ(result.newStatus, OrderStatus::CONFIRMED);
}

// -----------------------------------------------------------------------
// 출고 처리 (Phase 4에서 구현)
// -----------------------------------------------------------------------

TEST_F(OrderApproval, CONFIRMED_주문을_출고하면_RELEASED로_전환된다) {
    GTEST_SKIP() << "TODO: Phase 4 구현 후 작성";
}
