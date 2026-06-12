#include "MonitorView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
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
    std::cout << SEP << "===========================================" << RESET << "\n";
}

static void printDivider() {
    std::cout << SEP << "  -------------------------------------------" << RESET << "\n";
}

void MonitorView::showDashboard(const OrderStats&                      stats,
                                 const std::vector<SampleStockStatus>& stockStatuses) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "모니터링 대시보드" << RESET << "\n";
    printSep();

    // 주문 현황
    std::cout << "  " << LABEL << "[주문 현황]" << RESET << "\n";
    std::cout << "  RESERVED   : " << SECTION << std::setw(4) << stats.reserved  << "건" << RESET << "\n";
    std::cout << "  CONFIRMED  : " << SECTION << std::setw(4) << stats.confirmed << "건" << RESET << "\n";
    std::cout << "  PRODUCING  : " << SECTION << std::setw(4) << stats.producing << "건" << RESET << "\n";
    std::cout << "  RELEASED   : " << SECTION << std::setw(4) << stats.released  << "건" << RESET << "\n";
    printDivider();
    int total = stats.reserved + stats.confirmed + stats.producing + stats.released;
    std::cout << "  합계        : " << SECTION << std::setw(4) << total << "건" << RESET << "\n";
    printSep();

    // 재고 현황
    std::cout << "  " << LABEL << "[재고 현황]" << RESET << "\n";
    if (stockStatuses.empty()) {
        std::cout << "  " << SEP << "등록된 시료가 없습니다." << RESET << "\n";
    } else {
        std::cout << SEP << "  시료명                    재고        상태\n" << RESET;
        for (const auto& s : stockStatuses) {
            int pad = std::max(0, 24 - utf8DisplayWidth(s.sampleName));
            std::cout << "  " << std::left << s.sampleName
                      << std::string(pad, ' ')
                      << std::right << std::setw(6) << s.stock << " ea    ";
            switch (s.level) {
                case StockLevel::AMPLE:
                    std::cout << SUCCESS << "[여유]" << RESET; break;
                case StockLevel::SHORT:
                    std::cout << ERR     << "[부족]" << RESET; break;
                case StockLevel::EMPTY:
                    std::cout << SEP     << "[고갈]" << RESET; break;
            }
            std::cout << "\n";
        }
    }
    printSep();

    pauseForInput();
}
