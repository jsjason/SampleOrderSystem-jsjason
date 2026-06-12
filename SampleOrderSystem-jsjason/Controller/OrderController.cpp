#include "OrderController.h"

OrderController::OrderController(SampleRepository& sampleRepo,
                                 OrderRepository&  orderRepo,
                                 OrderView&        view)
    : sampleRepo_(sampleRepo), orderRepo_(orderRepo), view_(view) {}

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
