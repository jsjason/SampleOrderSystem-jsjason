#include <gtest/gtest.h>
#include <cmath>
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../Controller/OrderController.h"

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
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

TEST_F(OrderApproval, 생산_완료시_시료_재고가_실생산량만큼_증가한다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

TEST_F(OrderApproval, 생산_완료시_재고에서_주문량만큼_차감된다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

// -----------------------------------------------------------------------
// 출고 처리 (Phase 4에서 구현)
// -----------------------------------------------------------------------

TEST_F(OrderApproval, CONFIRMED_주문을_출고하면_RELEASED로_전환된다) {
    GTEST_SKIP() << "TODO: Phase 4 구현 후 작성";
}
