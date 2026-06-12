#pragma once
#include <string>
#include <vector>
#include <optional>
#include <ctime>
#include "../json.h"

struct ProductionJob {
    std::string orderNumber;
    std::string sampleId;
    int         actualQuantity;
    std::string enqueuedAt;
    std::string startedAt;      // 생산 시작 시각 (YYYY-MM-DD HH:MM:SS)
    double      totalDuration;  // 총 생산 소요 시간 (초 단위)
    int         creditedQuantity = 0; // 재고에 이미 반영된 수량

    nlohmann::json toJson() const;
    static ProductionJob fromJson(const nlohmann::json& j);
};

// 실 생산량 계산: ceil(부족분 / (수율 × 0.9))
int calculateActualQuantity(int shortage, double yield);

// 전방 선언 (processCompleted 파라미터용)
class SampleRepository;
class OrderRepository;

class ProductionQueue {
public:
    // filePath가 빈 문자열이면 파일 I/O 없이 in-memory로 동작 (테스트용).
    explicit ProductionQueue(const std::string& filePath);

    // 생산 작업 등록. avgProductionTime 단위: 분/ea.
    void enqueue(const std::string& orderNumber,
                 const std::string& sampleId,
                 int                actualQuantity,
                 double             avgProductionTime);

    // 큐 앞 작업 조회 (제거하지 않음). 비어있으면 std::nullopt.
    std::optional<ProductionJob> front() const;

    // 큐 앞 작업 제거.
    void dequeue();

    // 대기 중인 전체 작업 목록 반환.
    std::vector<ProductionJob> getAll() const;

    bool empty() const;

    // 경과 시간 기준으로 완료된 작업을 FIFO 순서대로 처리.
    // now 기본값은 실제 시스템 시간. 테스트에서는 타임스탬프를 직접 주입.
    void processCompleted(SampleRepository& sampleRepo,
                          OrderRepository&  orderRepo,
                          std::time_t       now = std::time(nullptr));

private:
    std::string                filePath_;
    std::vector<ProductionJob> jobs_;

    void        load();
    void        save();
    std::string currentDateTimeString();
};
