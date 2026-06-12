#include "MainView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <limits>

using namespace Color;

void MainView::showMenu(int sampleCount, int totalStock,
                         int orderCount,  int queueCount) const {
    clearScreen();

    std::time_t now = std::time(nullptr);
    char timeBuf[20];
    std::tm localTm{};
#ifdef _WIN32
    localtime_s(&localTm, &now);
#else
    localtime_r(&now, &localTm);
#endif
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &localTm);

    std::cout << SEP << "===========================================" << RESET << "\n";
    std::cout << "  " << TITLE << "S-Semi 시료 생산주문관리 시스템" << RESET << "\n";
    std::cout << "  " << SEP << timeBuf << RESET << "\n";
    std::cout << "  시료: " << SECTION << sampleCount << "개" << RESET
              << "    총 재고: " << SECTION << totalStock << " ea" << RESET
              << "    주문: "    << SECTION << orderCount << "건" << RESET
              << "    생산 대기: " << SECTION << queueCount << "건" << RESET << "\n";
    std::cout << SEP << "===========================================" << RESET << "\n";
    std::cout << "  " << LABEL << "[1]" << RESET << " 시료 관리\n";
    std::cout << "  " << LABEL << "[2]" << RESET << " 시료 주문\n";
    std::cout << "  " << LABEL << "[3]" << RESET << " 주문 승인/거절\n";
    std::cout << "  " << LABEL << "[4]" << RESET << " 모니터링\n";
    std::cout << "  " << LABEL << "[5]" << RESET << " 생산라인 조회\n";
    std::cout << "  " << LABEL << "[6]" << RESET << " 출고 처리\n";
    std::cout << "  " << LABEL << "[0]" << RESET << " 종료\n";
    std::cout << SEP << "===========================================" << RESET << "\n";
}

int MainView::promptMenuChoice() const {
    std::cout << PROMPT << "선택 > " << RESET;
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;  // 추출 실패 시 C++11은 0으로 덮어쓰므로 명시적으로 복원
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void MainView::showNotImplemented() const {
    std::cout << "\n  " << SEP << "아직 구현되지 않은 기능입니다." << RESET << "\n";
    pauseForInput();
}

void MainView::showInvalidInput() const {
    std::cout << "\n  " << ERR << "잘못된 입력입니다. 메뉴 번호를 다시 입력하세요." << RESET << "\n";
    pauseForInput();
}
