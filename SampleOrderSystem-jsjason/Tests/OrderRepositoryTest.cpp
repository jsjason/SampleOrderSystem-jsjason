#include <gtest/gtest.h>
#include <regex>
#include "../Model/Order.h"

namespace {
    Order placeOrder(OrderRepository& repo,
                     const std::string& sampleId  = "S-001",
                     const std::string& customer   = "테스트 고객",
                     int quantity = 100) {
        return repo.add(sampleId, customer, quantity);
    }
}

// -----------------------------------------------------------------------
// 주문 접수
// -----------------------------------------------------------------------

TEST(OrderRepository, 주문을_접수하면_RESERVED_상태로_저장된다) {
    OrderRepository repo("");
    Order o = placeOrder(repo);
    EXPECT_EQ(o.status, OrderStatus::RESERVED);
    auto found = repo.findByNumber(o.orderNumber);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->status, OrderStatus::RESERVED);
}

TEST(OrderRepository, 주문번호는_ORD_날짜_시퀀스_형식으로_생성된다) {
    OrderRepository repo("");
    Order o = placeOrder(repo);
    std::regex pattern(R"(ORD-\d{8}-\d{4})");
    EXPECT_TRUE(std::regex_match(o.orderNumber, pattern));
}

// -----------------------------------------------------------------------
// 주문 조회
// -----------------------------------------------------------------------

TEST(OrderRepository, 전체_주문_목록을_반환한다) {
    OrderRepository repo("");
    placeOrder(repo, "S-001");
    placeOrder(repo, "S-002");
    EXPECT_EQ(repo.getAll().size(), 2u);
}

TEST(OrderRepository, 주문번호로_특정_주문을_조회한다) {
    OrderRepository repo("");
    Order o = placeOrder(repo, "S-001", "삼성전자 파운드리", 200);
    auto found = repo.findByNumber(o.orderNumber);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->sampleId,     "S-001");
    EXPECT_EQ(found->customerName, "삼성전자 파운드리");
    EXPECT_EQ(found->quantity,     200);
}

TEST(OrderRepository, RESERVED_상태_주문만_필터링한다) {
    OrderRepository repo("");
    placeOrder(repo, "S-001");
    placeOrder(repo, "S-002");
    auto result = repo.filterByStatus(OrderStatus::RESERVED);
    EXPECT_EQ(result.size(), 2u);
}

TEST(OrderRepository, CONFIRMED_상태_주문만_필터링한다) {
    OrderRepository repo("");
    Order o1 = placeOrder(repo, "S-001");
    Order o2 = placeOrder(repo, "S-002");
    repo.updateStatus(o1.orderNumber, OrderStatus::CONFIRMED);
    auto confirmed = repo.filterByStatus(OrderStatus::CONFIRMED);
    auto reserved  = repo.filterByStatus(OrderStatus::RESERVED);
    ASSERT_EQ(confirmed.size(), 1u);
    EXPECT_EQ(confirmed[0].orderNumber, o1.orderNumber);
    EXPECT_EQ(reserved.size(), 1u);
    EXPECT_EQ(reserved[0].orderNumber, o2.orderNumber);
}

// -----------------------------------------------------------------------
// 상태 전환
// -----------------------------------------------------------------------

TEST(OrderRepository, RESERVED에서_CONFIRMED로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = placeOrder(repo);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::CONFIRMED));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::CONFIRMED);
}

TEST(OrderRepository, RESERVED에서_PRODUCING으로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = placeOrder(repo);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::PRODUCING));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::PRODUCING);
}

TEST(OrderRepository, RESERVED에서_REJECTED로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = placeOrder(repo);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::REJECTED));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::REJECTED);
}

TEST(OrderRepository, CONFIRMED에서_RELEASED로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = placeOrder(repo);
    repo.updateStatus(o.orderNumber, OrderStatus::CONFIRMED);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::RELEASED));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::RELEASED);
}

TEST(OrderRepository, PRODUCING에서_CONFIRMED로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = placeOrder(repo);
    repo.updateStatus(o.orderNumber, OrderStatus::PRODUCING);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::CONFIRMED));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::CONFIRMED);
}
