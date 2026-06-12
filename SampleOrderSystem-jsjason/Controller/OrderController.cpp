#include "OrderController.h"
#include <algorithm>

// -----------------------------------------------------------------------
// 자유 함수 — 승인/거절 핵심 로직
// -----------------------------------------------------------------------

ApprovalResult applyApproval(const std::string& orderNumber,
                              SampleRepository&  sampleRepo,
                              OrderRepository&   orderRepo,
                              ProductionQueue&   prodQueue) {
    auto order  = orderRepo.findByNumber(orderNumber).value();
    auto sample = sampleRepo.findById(order.sampleId).value();

    // 생산 중인 주문들이 완료 시 차감할 수량 합산 → 해당 재고는 이미 선점됨
    int reserved = 0;
    for (const auto& job : prodQueue.getAll()) {
        auto pendingOrder = orderRepo.findByNumber(job.orderNumber);
        if (pendingOrder.has_value())
            reserved += pendingOrder->quantity;
    }
    int availableStock = std::max(0, sample.stock - reserved);

    if (availableStock >= order.quantity) {
        sampleRepo.deductStock(order.sampleId, order.quantity);
        orderRepo.updateStatus(orderNumber, OrderStatus::CONFIRMED);
        return { OrderStatus::CONFIRMED, 0, 0 };
    }

    int shortage  = order.quantity - availableStock;
    int actualQty = calculateActualQuantity(shortage, sample.yield);
    prodQueue.enqueue(orderNumber, order.sampleId, actualQty, sample.avgProductionTime);
    orderRepo.updateStatus(orderNumber, OrderStatus::PRODUCING);
    return { OrderStatus::PRODUCING, shortage, actualQty };
}

bool applyRejection(const std::string& orderNumber,
                    OrderRepository&   orderRepo) {
    return orderRepo.updateStatus(orderNumber, OrderStatus::REJECTED);
}

// -----------------------------------------------------------------------
// OrderController
// -----------------------------------------------------------------------

OrderController::OrderController(SampleRepository& sampleRepo,
                                 OrderRepository&  orderRepo,
                                 ProductionQueue&  prodQueue,
                                 OrderView&        view)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , view_(view) {}

void OrderController::run() {
    while (true) {
        view_.showMenu();
        int choice = view_.promptMenuChoice();
        switch (choice) {
            case 1: handlePlaceOrder(); break;
            case 2: handleListOrders(); break;
            case 0: return;
            default: view_.showInvalidInput(); break;
        }
    }
}

void OrderController::runApproval() {
    prodQueue_.processCompleted(sampleRepo_, orderRepo_);
    auto orders = orderRepo_.filterByStatus(OrderStatus::RESERVED);
    if (orders.empty()) {
        view_.showNoReservedOrders();
        return;
    }

    std::vector<std::string> sampleNames;
    for (const auto& o : orders) {
        auto s = sampleRepo_.findById(o.sampleId);
        sampleNames.push_back(s.has_value() ? s->name : o.sampleId);
    }
    view_.showReservedList(orders, sampleNames);
    int idx = view_.promptOrderSelection(static_cast<int>(orders.size()));
    if (idx == 0) return;
    if (idx < 1 || idx > static_cast<int>(orders.size())) {
        view_.showInvalidInput();
        return;
    }

    const Order& order  = orders[idx - 1];
    auto         sample = sampleRepo_.findById(order.sampleId).value();

    int reserved = 0;
    for (const auto& job : prodQueue_.getAll()) {
        auto pending = orderRepo_.findByNumber(job.orderNumber);
        if (pending.has_value())
            reserved += pending->quantity;
    }
    int availableStock = std::max(0, sample.stock - reserved);
    int decision = view_.promptApprovalDecision(order, sample, availableStock);
    switch (decision) {
        case 1: handleApprove(order); break;
        case 2: handleReject(order);  break;
        case 0: return;
        default: view_.showInvalidInput(); break;
    }
}

void OrderController::handlePlaceOrder() {
    std::string sampleId = view_.promptSampleId();
    if (!sampleRepo_.findById(sampleId).has_value()) {
        view_.showOrderFail("등록되지 않은 시료 ID입니다: " + sampleId);
        return;
    }
    auto input = view_.promptOrderInput(sampleId);
    if (input.quantity <= 0) {
        view_.showOrderFail("수량은 1 이상의 정수여야 합니다.");
        return;
    }
    Order o = orderRepo_.add(input.sampleId, input.customerName, input.quantity);
    view_.showOrderSuccess(o);
}

void OrderController::handleListOrders() {
    auto orders = orderRepo_.getAll();
    if (orders.empty())
        view_.showEmpty();
    else
        view_.showList(orders);
}

void OrderController::handleApprove(const Order& order) {
    auto result = applyApproval(order.orderNumber, sampleRepo_, orderRepo_, prodQueue_);
    if (result.newStatus == OrderStatus::CONFIRMED)
        view_.showApprovalConfirmed(order);
    else
        view_.showApprovalProducing(order, result.shortage, result.actualQuantity);
}

void OrderController::handleReject(const Order& order) {
    applyRejection(order.orderNumber, orderRepo_);
    view_.showRejected(order);
}
