#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../json.h"

struct ProductionJob {
    std::string orderNumber;
    std::string sampleId;
    int         actualQuantity;
    std::string enqueuedAt;
    std::string startedAt;      // Phase 3b에서 사용. 3a에서는 빈 문자열.
    double      totalDuration;  // Phase 3b에서 사용. 3a에서는 0.0.

    nlohmann::json toJson() const;
    static ProductionJob fromJson(const nlohmann::json& j);
};

// 실 생산량 계산: ceil(부족분 / (수율 × 0.9))
int calculateActualQuantity(int shortage, double yield);

class ProductionQueue {
public:
    // filePath가 빈 문자열이면 파일 I/O 없이 in-memory로 동작 (테스트용).
    explicit ProductionQueue(const std::string& filePath);

    // 생산 작업 등록 (큐 뒤에 추가).
    void enqueue(const std::string& orderNumber,
                 const std::string& sampleId,
                 int actualQuantity);

    // 큐 앞 작업 조회 (제거하지 않음). 비어있으면 std::nullopt.
    std::optional<ProductionJob> front() const;

    // 큐 앞 작업 제거.
    void dequeue();

    // 대기 중인 전체 작업 목록 반환.
    std::vector<ProductionJob> getAll() const;

    bool empty() const;

private:
    std::string                filePath_;
    std::vector<ProductionJob> jobs_;

    void        load();
    void        save();
    std::string currentDateTimeString();
};
