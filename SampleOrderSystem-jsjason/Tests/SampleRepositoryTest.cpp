#include <gtest/gtest.h>
#include "../Model/Sample.h"

namespace {
    Sample makeSample(const std::string& id   = "S-001",
                      const std::string& name = "테스트 시료",
                      int stock = 100) {
        return { id, name, 0.5, 0.92, stock };
    }
}

// -----------------------------------------------------------------------
// 시료 등록
// -----------------------------------------------------------------------

TEST(SampleRepository, 시료를_등록하면_목록에서_조회된다) {
    SampleRepository repo("");
    EXPECT_TRUE(repo.add(makeSample()));
    auto result = repo.findById("S-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "S-001");
    EXPECT_EQ(result->name, "테스트 시료");
}

TEST(SampleRepository, 중복_ID로_등록하면_실패한다) {
    SampleRepository repo("");
    EXPECT_TRUE(repo.add(makeSample("S-001")));
    EXPECT_FALSE(repo.add(makeSample("S-001")));
    EXPECT_EQ(repo.getAll().size(), 1u);
}

// -----------------------------------------------------------------------
// 시료 조회
// -----------------------------------------------------------------------

TEST(SampleRepository, 등록된_모든_시료_목록을_반환한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001"));
    repo.add(makeSample("S-002"));
    EXPECT_EQ(repo.getAll().size(), 2u);
}

TEST(SampleRepository, ID로_특정_시료를_조회한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "웨이퍼 시료", 200));
    auto result = repo.findById("S-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "웨이퍼 시료");
    EXPECT_EQ(result->stock, 200);
}

TEST(SampleRepository, 존재하지_않는_ID_조회시_nullopt를_반환한다) {
    SampleRepository repo("");
    EXPECT_FALSE(repo.findById("X-999").has_value());
}

// -----------------------------------------------------------------------
// 시료 검색
// -----------------------------------------------------------------------

TEST(SampleRepository, 이름_키워드로_시료를_검색한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "실리콘 웨이퍼-8인치"));
    repo.add(makeSample("S-002", "GaN 에피택셜-4인치"));
    auto results = repo.searchByName("웨이퍼");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S-001");
}

TEST(SampleRepository, 대소문자_무관하게_이름을_검색한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "Silicon Wafer 8inch"));
    repo.add(makeSample("S-002", "GaN Epitaxial 4inch"));
    auto results = repo.searchByName("silicon");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S-001");
}

// -----------------------------------------------------------------------
// 재고 관리
// -----------------------------------------------------------------------

TEST(SampleRepository, 재고를_차감하면_stock이_감소한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "테스트", 100));
    EXPECT_TRUE(repo.deductStock("S-001", 30));
    EXPECT_EQ(repo.findById("S-001")->stock, 70);
}

TEST(SampleRepository, 재고보다_많은_수량을_차감하면_실패한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "테스트", 50));
    EXPECT_FALSE(repo.deductStock("S-001", 100));
    EXPECT_EQ(repo.findById("S-001")->stock, 50);
}

TEST(SampleRepository, 재고를_추가하면_stock이_증가한다) {
    SampleRepository repo("");
    repo.add(makeSample("S-001", "테스트", 50));
    repo.addStock("S-001", 200);
    EXPECT_EQ(repo.findById("S-001")->stock, 250);
}
