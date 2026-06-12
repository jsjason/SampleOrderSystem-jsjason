#include "OrderView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
#include <limits>

using namespace Color;

static void printSep() {
    std::cout << SEP << "-------------------------------------------" << RESET << "\n";
}

static std::string statusLabel(OrderStatus s) {
    switch (s) {
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::RELEASED:  return "RELEASED";
        default:                     return "RESERVED";
    }
}

void OrderView::showMenu() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "시료 주문" << RESET << "\n";
    printSep();
    std::cout << "  " << LABEL << "[1]" << RESET << " 주문 접수\n";
    std::cout << "  " << LABEL << "[2]" << RESET << " 주문 목록\n";
    std::cout << "  " << LABEL << "[0]" << RESET << " 돌아가기\n";
    printSep();
}

int OrderView::promptMenuChoice() const {
    std::cout << PROMPT << "선택 > " << RESET;
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;  // 추출 실패 시 C++11은 0으로 덮어쓰므로 명시적으로 복원
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

std::string OrderView::promptSampleId() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "주문 접수" << RESET << "\n";
    printSep();
    std::cout << SEP << "시료 ID        : " << RESET;
    std::string sampleId;
    std::getline(std::cin, sampleId);
    return sampleId;
}

OrderView::OrderInput OrderView::promptOrderInput(const std::string& sampleId) const {
    OrderInput input{};
    input.sampleId = sampleId;

    std::cout << SEP << "고객명          : " << RESET;
    std::getline(std::cin, input.customerName);

    std::cout << SEP << "주문 수량 (ea) : " << RESET;
    std::string line;
    std::getline(std::cin, line);
    try { input.quantity = std::stoi(line); } catch (...) { input.quantity = 0; }

    return input;
}

void OrderView::showOrderSuccess(const Order& o) const {
    std::cout << "\n  " << SUCCESS << "주문이 접수되었습니다." << RESET << "\n";
    std::cout << "  " << SEP << "주문번호 : " << RESET << LABEL << o.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "시료 ID  : " << RESET << o.sampleId << "\n";
    std::cout << "  " << SEP << "고객명    : " << RESET << o.customerName << "\n";
    std::cout << "  " << SEP << "수량      : " << RESET << SECTION << o.quantity << " ea" << RESET << "\n";
    std::cout << "  " << SEP << "상태      : " << RESET << statusLabel(o.status) << "\n";
    printSep();
    pauseForInput();
}

void OrderView::showOrderFail(const std::string& reason) const {
    std::cout << "\n  " << ERR << "오류: " << reason << RESET << "\n";
    printSep();
    pauseForInput();
}

void OrderView::showList(const std::vector<Order>& orders) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "주문 목록" << RESET
              << SEP << "  (" << orders.size() << "건)" << RESET << "\n";
    printSep();

    // 헤더 (Korean 글자는 2 display col이므로 수작업으로 간격 맞춤)
    // 열 너비: 주문번호 20 | 시료ID 9 | 수량 10 | 상태 13 | 접수일시 19
    std::cout << SEP
              << "  주문번호            시료ID   수량      상태         접수일시\n"
              << "  " << std::string(71, '-') << RESET << "\n";

    for (const auto& o : orders) {
        std::cout << "  "
                  << LABEL << std::left << std::setw(20) << o.orderNumber << RESET
                  << std::left << std::setw(9)  << o.sampleId
                  << std::right << std::setw(5) << o.quantity << " ea  "
                  << std::left << std::setw(13) << statusLabel(o.status)
                  << SEP << o.createdAt << RESET << "\n";
        std::cout << SEP << "    고객명: " << RESET << o.customerName << "\n";
    }

    printSep();
    pauseForInput();
}

void OrderView::showEmpty() const {
    clearScreen();
    printSep();
    std::cout << "  " << SEP << "접수된 주문이 없습니다." << RESET << "\n";
    printSep();
    pauseForInput();
}

void OrderView::showInvalidInput() const {
    std::cout << "\n  " << ERR << "잘못된 입력입니다. 메뉴 번호를 다시 입력하세요." << RESET << "\n";
    pauseForInput();
}
