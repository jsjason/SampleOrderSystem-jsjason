#include <gtest/gtest.h>

// TODO: #include "../Model/Sample.h"

// -----------------------------------------------------------------------
// 시료 등록
// -----------------------------------------------------------------------

TEST(SampleRepository, 시료를_등록하면_목록에서_조회된다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, 중복_ID로_등록하면_실패한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

// -----------------------------------------------------------------------
// 시료 조회
// -----------------------------------------------------------------------

TEST(SampleRepository, 등록된_모든_시료_목록을_반환한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, ID로_특정_시료를_조회한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, 존재하지_않는_ID_조회시_nullopt를_반환한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

// -----------------------------------------------------------------------
// 시료 검색
// -----------------------------------------------------------------------

TEST(SampleRepository, 이름_키워드로_시료를_검색한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, 대소문자_무관하게_이름을_검색한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

// -----------------------------------------------------------------------
// 재고 관리
// -----------------------------------------------------------------------

TEST(SampleRepository, 재고를_차감하면_stock이_감소한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, 재고보다_많은_수량을_차감하면_실패한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}

TEST(SampleRepository, 재고를_추가하면_stock이_증가한다) {
    GTEST_SKIP() << "TODO: Sample 모델 구현 후 작성";
}
