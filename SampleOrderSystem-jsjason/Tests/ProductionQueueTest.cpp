#include <gtest/gtest.h>
#include "../Model/ProductionQueue.h"

// -----------------------------------------------------------------------
// 실 생산량 계산 (Phase 3b에서 구현)
// 공식: ceil(부족분 / (수율 * 0.9))
// -----------------------------------------------------------------------

TEST(ProductionCalculation, 부족분과_수율로_실_생산량을_계산한다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

TEST(ProductionCalculation, 수율_0_92_부족분_170일때_실생산량은_206이다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
    // ceil(170 / (0.92 * 0.9)) = ceil(170 / 0.828) = ceil(205.31) = 206
}

TEST(ProductionCalculation, 총_생산시간은_평균생산시간_분_곱하기_실생산량_곱하기_60초이다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

TEST(ProductionCalculation, 부족분이_0이면_생산_작업을_등록하지_않는다) {
    GTEST_SKIP() << "TODO: Phase 3b 구현 후 작성";
}

// -----------------------------------------------------------------------
// FIFO 큐 동작
// -----------------------------------------------------------------------

TEST(ProductionQueue, 작업을_등록하면_큐_뒤에_추가된다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100);
    ASSERT_EQ(q.getAll().size(), 1u);
    EXPECT_EQ(q.getAll().back().orderNumber, "ORD-20260612-0001");
}

TEST(ProductionQueue, 먼저_등록된_작업이_먼저_처리된다_FIFO) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100);
    q.enqueue("ORD-20260612-0002", "S-002",  50);
    ASSERT_TRUE(q.front().has_value());
    EXPECT_EQ(q.front()->orderNumber, "ORD-20260612-0001");
}

TEST(ProductionQueue, 빈_큐에서_처리할_작업이_없으면_nullopt를_반환한다) {
    ProductionQueue q("");
    EXPECT_FALSE(q.front().has_value());
}

TEST(ProductionQueue, 작업_완료시_큐에서_제거된다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100);
    q.dequeue();
    EXPECT_TRUE(q.empty());
}

TEST(ProductionQueue, 대기_중인_작업_목록을_반환한다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100);
    q.enqueue("ORD-20260612-0002", "S-002",  50);
    EXPECT_EQ(q.getAll().size(), 2u);
}
