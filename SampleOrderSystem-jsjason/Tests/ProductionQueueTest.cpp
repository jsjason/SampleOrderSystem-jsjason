#include <gtest/gtest.h>
#include <cstdio>
#include <ctime>
#include "../Model/ProductionQueue.h"
#include "../Model/Sample.h"
#include "../Model/Order.h"

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

// -----------------------------------------------------------------------
// 실 생산량 계산
// 공식: ceil(부족분 / (수율 * 0.9))
// -----------------------------------------------------------------------

TEST(ProductionCalculation, 부족분과_수율로_실_생산량을_계산한다) {
    int actual = calculateActualQuantity(100, 0.92);
    EXPECT_GT(actual, 100);
}

TEST(ProductionCalculation, 수율_0_92_부족분_170일때_실생산량은_206이다) {
    // ceil(170 / (0.92 * 0.9)) = ceil(170 / 0.828) = ceil(205.31) = 206
    EXPECT_EQ(calculateActualQuantity(170, 0.92), 206);
}

TEST(ProductionCalculation, 총_생산시간은_평균생산시간_분_곱하기_실생산량_곱하기_60초이다) {
    ProductionQueue q("");
    q.enqueue("ORD-001", "S-001", 10, 0.5);  // 0.5분/ea * 10ea * 60 = 300초
    ASSERT_TRUE(q.front().has_value());
    EXPECT_DOUBLE_EQ(q.front()->totalDuration, 300.0);
}

TEST(ProductionCalculation, 부족분이_0이면_실생산량은_0이다) {
    EXPECT_EQ(calculateActualQuantity(0, 0.92), 0);
}

// -----------------------------------------------------------------------
// FIFO 큐 동작
// -----------------------------------------------------------------------

TEST(ProductionQueue, 작업을_등록하면_큐_뒤에_추가된다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100, 0.0);
    ASSERT_EQ(q.getAll().size(), 1u);
    EXPECT_EQ(q.getAll().back().orderNumber, "ORD-20260612-0001");
}

TEST(ProductionQueue, 먼저_등록된_작업이_먼저_처리된다_FIFO) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100, 0.0);
    q.enqueue("ORD-20260612-0002", "S-002",  50, 0.0);
    ASSERT_TRUE(q.front().has_value());
    EXPECT_EQ(q.front()->orderNumber, "ORD-20260612-0001");
}

TEST(ProductionQueue, 빈_큐에서_처리할_작업이_없으면_nullopt를_반환한다) {
    ProductionQueue q("");
    EXPECT_FALSE(q.front().has_value());
}

TEST(ProductionQueue, 작업_완료시_큐에서_제거된다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100, 0.0);
    q.dequeue();
    EXPECT_TRUE(q.empty());
}

TEST(ProductionQueue, 대기_중인_작업_목록을_반환한다) {
    ProductionQueue q("");
    q.enqueue("ORD-20260612-0001", "S-001", 100, 0.0);
    q.enqueue("ORD-20260612-0002", "S-002",  50, 0.0);
    EXPECT_EQ(q.getAll().size(), 2u);
}

// -----------------------------------------------------------------------
// 자동 완료 처리
// -----------------------------------------------------------------------

TEST(ProductionQueue, 경과_시간이_충분하면_완료_작업이_자동_처리된다) {
    SampleRepository sampleRepo("");
    OrderRepository  orderRepo("");
    ProductionQueue  q("");

    sampleRepo.add(Sample{"S-001", "테스트", 0.01, 0.92, 0});
    auto order  = orderRepo.add("S-001", "테스트 고객", 5);
    int  actQty = calculateActualQuantity(5, 0.92);
    q.enqueue(order.orderNumber, "S-001", actQty, 0.01);

    auto job    = q.front().value();
    auto start  = parseTestTime(job.startedAt);
    auto future = start + static_cast<std::time_t>(job.totalDuration) + 1;
    q.processCompleted(sampleRepo, orderRepo, future);

    EXPECT_TRUE(q.empty());
}

TEST(ProductionQueue, 순차_처리시_다음_작업의_startedAt은_이전_작업_완료시각이다) {
    ProductionQueue q("");
    q.enqueue("ORD-001", "S-001", 10, 0.5);  // totalDuration = 300초
    q.enqueue("ORD-002", "S-002",  5, 0.3);

    auto jobs = q.getAll();
    ASSERT_EQ(jobs.size(), 2u);

    std::time_t start1 = parseTestTime(jobs[0].startedAt);
    std::time_t start2 = parseTestTime(jobs[1].startedAt);
    EXPECT_EQ(start2, start1 + static_cast<std::time_t>(jobs[0].totalDuration));
}

TEST(ProductionQueue, 경과_시간이_부족하면_작업이_처리되지_않는다) {
    SampleRepository sampleRepo("");
    OrderRepository  orderRepo("");
    ProductionQueue  q("");

    sampleRepo.add(Sample{"S-001", "테스트", 0.01, 0.92, 0});
    auto order  = orderRepo.add("S-001", "테스트 고객", 5);
    int  actQty = calculateActualQuantity(5, 0.92);
    q.enqueue(order.orderNumber, "S-001", actQty, 0.01);

    auto job   = q.front().value();
    auto start = parseTestTime(job.startedAt);
    auto past  = start + static_cast<std::time_t>(job.totalDuration) - 1;
    q.processCompleted(sampleRepo, orderRepo, past);

    EXPECT_FALSE(q.empty());
}
