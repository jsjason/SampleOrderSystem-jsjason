#include "MonitorController.h"

MonitorController::MonitorController(SampleRepository& sampleRepo,
                                     OrderRepository&  orderRepo,
                                     ProductionQueue&  prodQueue,
                                     MonitorView&      view)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , view_(view) {}

void MonitorController::run() {
    prodQueue_.processCompleted(sampleRepo_, orderRepo_);

    // 주문 현황 집계 (REJECTED 제외)
    OrderStats stats;
    for (const auto& o : orderRepo_.getAll()) {
        switch (o.status) {
            case OrderStatus::RESERVED:  stats.reserved++;  break;
            case OrderStatus::CONFIRMED: stats.confirmed++; break;
            case OrderStatus::PRODUCING: stats.producing++; break;
            case OrderStatus::RELEASED:  stats.released++;  break;
            default: break;
        }
    }

    // 시료별 재고 상태 판단
    std::vector<SampleStockStatus> stockStatuses;
    for (const auto& sample : sampleRepo_.getAll()) {
        // 미출고 전체 수요 (RESERVED + PRODUCING + CONFIRMED) 합산
        int totalDemand = 0;
        for (const auto& o : orderRepo_.getAll()) {
            if (o.sampleId == sample.id &&
                (o.status == OrderStatus::RESERVED ||
                 o.status == OrderStatus::PRODUCING ||
                 o.status == OrderStatus::CONFIRMED)) {
                totalDemand += o.quantity;
            }
        }

        StockLevel level;
        if (sample.stock == 0) {
            level = StockLevel::EMPTY;
        } else if (sample.stock < totalDemand) {
            level = StockLevel::SHORT;
        } else {
            level = StockLevel::AMPLE;
        }

        stockStatuses.push_back({ sample.id, sample.name, sample.stock, level });
    }

    view_.showDashboard(stats, stockStatuses);
}
