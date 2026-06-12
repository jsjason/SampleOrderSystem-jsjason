#include <gtest/gtest.h>

// TODO: #include "../Model/Order.h"

// -----------------------------------------------------------------------
// 주문 접수
// -----------------------------------------------------------------------

TEST(OrderRepository, 주문을_접수하면_RESERVED_상태로_저장된다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, 주문번호는_ORD_날짜_시퀀스_형식으로_생성된다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

// -----------------------------------------------------------------------
// 주문 조회
// -----------------------------------------------------------------------

TEST(OrderRepository, 전체_주문_목록을_반환한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, 주문번호로_특정_주문을_조회한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, RESERVED_상태_주문만_필터링한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, CONFIRMED_상태_주문만_필터링한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

// -----------------------------------------------------------------------
// 상태 전환
// -----------------------------------------------------------------------

TEST(OrderRepository, RESERVED에서_CONFIRMED로_상태를_변경한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, RESERVED에서_PRODUCING으로_상태를_변경한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, RESERVED에서_REJECTED로_상태를_변경한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, CONFIRMED에서_RELEASED로_상태를_변경한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}

TEST(OrderRepository, PRODUCING에서_CONFIRMED로_상태를_변경한다) {
    GTEST_SKIP() << "TODO: Order 모델 구현 후 작성";
}
