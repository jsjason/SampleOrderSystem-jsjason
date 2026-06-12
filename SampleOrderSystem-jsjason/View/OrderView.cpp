#include "OrderView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cstdint>

using namespace Color;

static int utf8DisplayWidth(const std::string& s) {
    int width = 0;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp = 0;
        int bytes = 1;
        if      (c < 0x80) { cp = c;        bytes = 1; }
        else if (c < 0xE0) { cp = c & 0x1F; bytes = 2; }
        else if (c < 0xF0) { cp = c & 0x0F; bytes = 3; }
        else               { cp = c & 0x07; bytes = 4; }
        for (int j = 1; j < bytes && i + j < s.size(); ++j)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
        i += bytes;
        if ((cp >= 0x1100 && cp <= 0x115F) ||
            (cp >= 0x2E80 && cp <= 0x9FFF) ||
            (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFF01 && cp <= 0xFF60) ||
            (cp >= 0xFFE0 && cp <= 0xFFE6))
            width += 2;
        else
            width += 1;
    }
    return width;
}

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

void OrderView::showReservedList(const std::vector<Order>& orders,
                                  const std::vector<std::string>& sampleNames) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "주문 승인/거절" << RESET
              << SEP << "  (" << orders.size() << "건)" << RESET << "\n";
    std::cout << SECTION
              << "  번호주문번호            수량      시료명                고객명              접수일시\n"
              << SEP << "  " << std::string(93, '-') << RESET << "\n";
    for (int i = 0; i < static_cast<int>(orders.size()); ++i) {
        const auto& o = orders[i];
        int namePad  = std::max(0, 20 - utf8DisplayWidth(sampleNames[i]));
        int custPad  = std::max(0, 16 - utf8DisplayWidth(o.customerName));
        std::cout << "  " << LABEL << "[" << (i + 1) << "]" << RESET << " "
                  << std::left << std::setw(20) << o.orderNumber
                  << std::right << std::setw(5) << o.quantity << " ea  "
                  << SECTION << sampleNames[i] << RESET
                  << std::string(namePad + 2, ' ') << o.customerName
                  << std::string(custPad  + 2, ' ') << SEP << o.createdAt << RESET << "\n";
    }
    printSep();
}

int OrderView::promptOrderSelection(int count) const {
    std::cout << PROMPT << "선택 (0=돌아가기) > " << RESET;
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    (void)count;
    return choice;
}

int OrderView::promptApprovalDecision(const Order& order, const Sample& sample) const {
    int shortage = order.quantity - sample.stock;

    printSep();
    std::cout << "  " << SECTION << "주문 상세" << RESET << "\n";
    printSep();
    std::cout << "  " << SEP << "주문번호      : " << RESET << LABEL << order.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "고객명        : " << RESET << order.customerName << "\n";
    std::cout << "  " << SEP << "주문 수량     : " << RESET << SECTION << order.quantity << " ea" << RESET << "\n";
    printSep();
    std::cout << "  " << SEP << "시료명        : " << RESET << sample.name
              << "  " << SEP << "(" << sample.id << ")" << RESET << "\n";
    std::cout << "  " << SEP << "현재 재고     : " << RESET << SECTION << sample.stock << " ea" << RESET << "\n";
    if (shortage > 0)
        std::cout << "  " << ERR  << "부족분        : " << shortage << " ea" << RESET << "\n";
    else
        std::cout << "  " << SUCCESS << "재고 충분" << RESET << "\n";
    std::cout << "  " << SEP << "평균 생산시간 : " << RESET
              << sample.avgProductionTime << " 분/ea" << "\n";
    printSep();
    std::cout << "  " << LABEL << "[1]" << RESET << " 승인\n";
    std::cout << "  " << LABEL << "[2]" << RESET << " 거절\n";
    std::cout << "  " << LABEL << "[0]" << RESET << " 취소\n";
    printSep();
    std::cout << PROMPT << "선택 > " << RESET;
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void OrderView::showApprovalConfirmed(const Order& order) const {
    std::cout << "\n  " << SUCCESS << "승인 완료: 재고가 확인되어 주문이 확정되었습니다." << RESET << "\n";
    std::cout << "  " << SEP << "주문번호 : " << RESET << LABEL << order.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "상태      : " << RESET << "CONFIRMED\n";
    printSep();
    pauseForInput();
}

void OrderView::showApprovalProducing(const Order& order, int shortage, int actualQty) const {
    std::cout << "\n  " << SUCCESS << "승인 완료: 재고 부족으로 생산이 등록되었습니다." << RESET << "\n";
    std::cout << "  " << SEP << "주문번호 : " << RESET << LABEL << order.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "상태      : " << RESET << "PRODUCING\n";
    std::cout << "  " << SEP << "부족분    : " << RESET
              << SECTION << std::right << std::setw(5) << shortage << " ea" << RESET
              << "  →  실 생산량: "
              << SECTION << actualQty << " ea" << RESET << "\n";
    printSep();
    pauseForInput();
}

void OrderView::showRejected(const Order& order) const {
    std::cout << "\n  " << ERR << "거절 완료: 주문이 거절 처리되었습니다." << RESET << "\n";
    std::cout << "  " << SEP << "주문번호 : " << RESET << LABEL << order.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "상태      : " << RESET << "REJECTED\n";
    printSep();
    pauseForInput();
}

void OrderView::showNoReservedOrders() const {
    clearScreen();
    printSep();
    std::cout << "  " << SEP << "승인 대기 중인 주문이 없습니다." << RESET << "\n";
    printSep();
    pauseForInput();
}
