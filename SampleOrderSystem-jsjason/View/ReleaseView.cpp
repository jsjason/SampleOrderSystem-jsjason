#include "ReleaseView.h"
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

void ReleaseView::showConfirmedList(const std::vector<Order>&        orders,
                                    const std::vector<std::string>& sampleNames) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "출고 처리 — 출고 대기 목록" << RESET
              << SEP << "  (" << orders.size() << "건)" << RESET << "\n";
    std::cout << SECTION
              << "  번호주문번호            수량      시료명                고객명\n"
              << SEP << "  " << std::string(79, '-') << RESET << "\n";

    for (int i = 0; i < static_cast<int>(orders.size()); ++i) {
        const auto& o    = orders[i];
        const auto& name = sampleNames[i];
        int namePad = std::max(0, 20 - utf8DisplayWidth(name));
        std::cout << "  " << LABEL << "[" << (i + 1) << "]" << RESET << " "
                  << std::left << std::setw(20) << o.orderNumber
                  << std::right << std::setw(5) << o.quantity << " ea  "
                  << SECTION << name << RESET
                  << std::string(namePad + 2, ' ') << o.customerName << "\n";
    }

    printSep();
    std::cout << "  " << LABEL << "[0]" << RESET << " 돌아가기\n";
    printSep();
}

void ReleaseView::showEmpty() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "출고 처리" << RESET << "\n";
    printSep();
    std::cout << "  " << SEP << "출고 대기 중인 주문이 없습니다." << RESET << "\n";
    printSep();
    pauseForInput();
}

void ReleaseView::showReleaseSuccess(const Order& order, const std::string& sampleName) const {
    printSep();
    std::cout << "  " << SUCCESS << "출고 완료" << RESET << "\n";
    printSep();
    std::cout << "  " << SEP << "주문번호 : " << RESET << LABEL << order.orderNumber << RESET << "\n";
    std::cout << "  " << SEP << "시료명   : " << RESET << sampleName << "\n";
    std::cout << "  " << SEP << "고객명   : " << RESET << order.customerName << "\n";
    std::cout << "  " << SEP << "수량     : " << RESET << SECTION << order.quantity << " ea" << RESET << "\n";
    std::cout << "  " << SEP << "상태     : " << RESET << "CONFIRMED → " << SUCCESS << "RELEASED" << RESET << "\n";
    printSep();
    pauseForInput();
}

void ReleaseView::showInvalidInput() const {
    std::cout << "\n  " << ERR << "잘못된 입력입니다. 메뉴 번호를 다시 입력하세요." << RESET << "\n";
    pauseForInput();
}

int ReleaseView::promptOrderSelection(int count) const {
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
