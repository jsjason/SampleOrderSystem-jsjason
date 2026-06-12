#include "ReleaseController.h"

ReleaseController::ReleaseController(SampleRepository& sampleRepo,
                                     OrderRepository&  orderRepo,
                                     ReleaseView&      view)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , view_(view) {}

void ReleaseController::run() {
    auto orders = orderRepo_.filterByStatus(OrderStatus::CONFIRMED);
    if (orders.empty()) {
        view_.showEmpty();
        return;
    }

    std::vector<std::string> sampleNames;
    for (const auto& o : orders) {
        auto s = sampleRepo_.findById(o.sampleId);
        sampleNames.push_back(s.has_value() ? s->name : o.sampleId);
    }

    view_.showConfirmedList(orders, sampleNames);
    int idx = view_.promptOrderSelection(static_cast<int>(orders.size()));

    if (idx == 0) return;
    if (idx < 1 || idx > static_cast<int>(orders.size())) {
        view_.showInvalidInput();
        return;
    }

    handleRelease(orders[idx - 1], sampleNames[idx - 1]);
}

void ReleaseController::handleRelease(const Order& order, const std::string& sampleName) {
    sampleRepo_.deductStock(order.sampleId, order.quantity);
    orderRepo_.updateStatus(order.orderNumber, OrderStatus::RELEASED);
    view_.showReleaseSuccess(order, sampleName);
}
