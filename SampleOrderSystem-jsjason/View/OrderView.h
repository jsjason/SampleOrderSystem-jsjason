#pragma once
#include <string>
#include <vector>
#include "../Model/Order.h"
#include "../Model/Sample.h"

class OrderView {
public:
    // --- Phase 2 ---
    void showMenu() const;
    int  promptMenuChoice() const;

    struct OrderInput {
        std::string sampleId;
        std::string customerName;
        int         quantity;
    };
    std::string promptSampleId() const;
    OrderInput  promptOrderInput(const std::string& sampleId) const;

    void showOrderSuccess(const Order& o) const;
    void showOrderFail(const std::string& reason) const;
    void showList(const std::vector<Order>& orders) const;
    void showEmpty() const;
    void showInvalidInput() const;

    // --- Phase 3a ---
    void showReservedList(const std::vector<Order>& orders,
                          const std::vector<std::string>& sampleNames) const;
    int  promptOrderSelection(int count) const;
    int  promptApprovalDecision(const Order& order, const Sample& sample,
                                int availableStock) const;
    void showApprovalConfirmed(const Order& order) const;
    void showApprovalProducing(const Order& order, int shortage, int actualQty) const;
    void showRejected(const Order& order) const;
    void showNoReservedOrders() const;
};
