#pragma once
#include <string>
#include <vector>
#include "../Model/Order.h"

class OrderView {
public:
    void showMenu() const;
    int  promptMenuChoice() const;

    struct OrderInput {
        std::string sampleId;
        std::string customerName;
        int         quantity;
    };
    // 시료 ID만 먼저 수집 (clearScreen + 폼 헤더 포함).
    std::string promptSampleId() const;
    // 시료 ID 검증 후 나머지 항목 수집 (화면 유지, clearScreen 없음).
    OrderInput  promptOrderInput(const std::string& sampleId) const;

    void showOrderSuccess(const Order& o) const;
    void showOrderFail(const std::string& reason) const;
    void showList(const std::vector<Order>& orders) const;
    void showEmpty() const;
    void showInvalidInput() const;
};
