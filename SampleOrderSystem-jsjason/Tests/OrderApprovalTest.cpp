#include <gtest/gtest.h>

// TODO: #include "../Model/Sample.h"
// TODO: #include "../Model/Order.h"
// TODO: #include "../Model/ProductionQueue.h"
// TODO: #include "../Controller/OrderController.h"

// -----------------------------------------------------------------------
// 주문 승인 - 재고 충분
// -----------------------------------------------------------------------

TEST(OrderApproval, 재고가_충분하면_승인시_CONFIRMED로_전환된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 재고가_충분하면_승인시_재고가_주문량만큼_차감된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 재고가_충분하면_승인시_생산_큐에_등록되지_않는다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

// -----------------------------------------------------------------------
// 주문 승인 - 재고 부족
// -----------------------------------------------------------------------

TEST(OrderApproval, 재고가_부족하면_승인시_PRODUCING으로_전환된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 재고가_부족하면_승인시_생산_큐에_자동_등록된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 재고가_부족하면_부족분만큼_생산_작업이_생성된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 재고가_0이면_주문량_전체가_생산_대상이다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

// -----------------------------------------------------------------------
// 주문 거절
// -----------------------------------------------------------------------

TEST(OrderApproval, 거절하면_REJECTED로_전환된다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

TEST(OrderApproval, 거절해도_재고는_변경되지_않는다) {
    GTEST_SKIP() << "TODO: OrderController 구현 후 작성";
}

// -----------------------------------------------------------------------
// 생산 완료 처리
// -----------------------------------------------------------------------

TEST(OrderApproval, 생산_완료시_PRODUCING에서_CONFIRMED로_전환된다) {
    GTEST_SKIP() << "TODO: ProductionController 구현 후 작성";
}

TEST(OrderApproval, 생산_완료시_시료_재고가_실생산량만큼_증가한다) {
    GTEST_SKIP() << "TODO: ProductionController 구현 후 작성";
}

TEST(OrderApproval, 생산_완료시_재고에서_주문량만큼_차감된다) {
    GTEST_SKIP() << "TODO: ProductionController 구현 후 작성";
}

// -----------------------------------------------------------------------
// 출고 처리
// -----------------------------------------------------------------------

TEST(OrderApproval, CONFIRMED_주문을_출고하면_RELEASED로_전환된다) {
    GTEST_SKIP() << "TODO: ReleaseController 구현 후 작성";
}
