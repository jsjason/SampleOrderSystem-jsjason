#include "DummyView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <limits>

using namespace Color;

bool DummyView::promptConfirm() const {
    clearScreen();
    std::cout << SEP << "===========================================" << RESET << "\n";
    std::cout << "  " << TITLE << "더미 데이터 생성" << RESET << "\n";
    std::cout << SEP << "===========================================" << RESET << "\n";
    std::cout << "  " << ERR << "기존 데이터(시료, 주문, 생산 큐)가 모두 초기화됩니다." << RESET << "\n";
    std::cout << SEP << "===========================================" << RESET << "\n";
    std::cout << PROMPT << "  계속하시겠습니까? (y/n) > " << RESET;

    char c = 0;
    std::cin >> c;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return c == 'y' || c == 'Y';
}

void DummyView::showSuccess(int sampleCount, int orderCount) const {
    std::cout << "\n  " << SUCCESS << "더미 데이터가 생성되었습니다." << RESET << "\n";
    std::cout << "  " << SECTION << "시료 " << sampleCount << "개 / 주문 " << orderCount << "건이 등록되었습니다." << RESET << "\n";
    pauseForInput();
}
