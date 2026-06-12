#include "SampleView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
#include <limits>

using namespace Color;

static void printSep() {
    std::cout << SEP << "-------------------------------------------" << RESET << "\n";
}

void SampleView::showMenu() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "시료 관리" << RESET << "\n";
    printSep();
    std::cout << "  " << LABEL << "[1]" << RESET << " 시료 등록\n";
    std::cout << "  " << LABEL << "[2]" << RESET << " 시료 목록\n";
    std::cout << "  " << LABEL << "[3]" << RESET << " 이름 검색\n";
    std::cout << "  " << LABEL << "[0]" << RESET << " 돌아가기\n";
    printSep();
}

int SampleView::promptMenuChoice() const {
    std::cout << PROMPT << "선택 > " << RESET;
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;  // 추출 실패 시 C++11은 0으로 덮어쓰므로 명시적으로 복원
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

Sample SampleView::promptRegisterInput() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "시료 등록" << RESET << "\n";
    printSep();

    Sample s{};

    std::cout << SEP << "시료 ID              : " << RESET;
    std::getline(std::cin, s.id);

    std::cout << SEP << "시료명               : " << RESET;
    std::getline(std::cin, s.name);

    std::string line;
    std::cout << SEP << "평균 생산시간(min/ea) : " << RESET;
    std::getline(std::cin, line);
    try { s.avgProductionTime = std::stod(line); } catch (...) { s.avgProductionTime = 0.0; }

    std::cout << SEP << "수율 (0.0~1.0)       : " << RESET;
    std::getline(std::cin, line);
    try { s.yield = std::stod(line); } catch (...) { s.yield = 0.0; }

    std::cout << SEP << "초기 재고 (ea)       : " << RESET;
    std::getline(std::cin, line);
    try { s.stock = std::stoi(line); } catch (...) { s.stock = 0; }

    return s;
}

std::string SampleView::promptSearchKeyword() const {
    std::cout << "\n" << PROMPT << "검색어 입력 > " << RESET;
    std::string keyword;
    std::getline(std::cin, keyword);
    return keyword;
}

void SampleView::printTable(const std::vector<Sample>& samples) const {
    std::cout << std::left
              << "  " << std::setw(8)  << "ID"
              << "  " << std::setw(24) << "이름"
              << "  " << std::setw(10) << "생산시간"
              << "  " << std::setw(6)  << "수율"
              << "  재고\n";
    std::cout << SEP
              << "  " << std::string(8,  '-')
              << "  " << std::string(24, '-')
              << "  " << std::string(10, '-')
              << "  " << std::string(6,  '-')
              << "  " << std::string(8,  '-')
              << RESET << "\n";

    for (const auto& s : samples) {
        std::cout << "  " << std::left  << std::setw(8)  << s.id
                  << "  " << std::left  << s.name << "\n";
        std::cout << "  " << std::string(10, ' ')
                  << "  " << std::string(24, ' ')
                  << "  " << std::right << std::fixed << std::setprecision(2)
                  << std::setw(6) << s.avgProductionTime << " min"
                  << "  " << std::setw(5) << s.yield
                  << "  " << std::setw(6) << s.stock << " ea\n";
    }
}

void SampleView::showList(const std::vector<Sample>& samples) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "등록 시료 목록" << RESET
              << SEP << "  (" << samples.size() << "건)" << RESET << "\n";
    printSep();
    for (const auto& s : samples) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  " << LABEL << "[" << s.id << "]" << RESET
                  << "  " << s.name
                  << SEP << "  |  " << s.avgProductionTime << " min/ea"
                  << "  |  수율 " << s.yield
                  << "  |  재고 " << RESET << SECTION << s.stock << " ea" << RESET << "\n";
    }
    printSep();
    pauseForInput();
}

void SampleView::showSearchResult(const std::vector<Sample>& samples,
                                  const std::string& keyword) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "검색 결과" << RESET
              << SEP << ": \"" << keyword << "\"  (" << samples.size() << "건)" << RESET << "\n";
    printSep();
    if (samples.empty()) {
        std::cout << "  " << SEP << "일치하는 시료가 없습니다." << RESET << "\n";
    } else {
        for (const auto& s : samples) {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "  " << LABEL << "[" << s.id << "]" << RESET
                      << "  " << s.name
                      << SEP << "  |  " << s.avgProductionTime << " min/ea"
                      << "  |  수율 " << s.yield
                      << "  |  재고 " << RESET << SECTION << s.stock << " ea" << RESET << "\n";
        }
    }
    printSep();
    pauseForInput();
}

void SampleView::showRegisterSuccess(const Sample& s) const {
    std::cout << "\n  " << SUCCESS << "시료가 등록되었습니다: "
              << LABEL << "[" << s.id << "]" << SUCCESS
              << " " << s.name << RESET << "\n";
    pauseForInput();
}

void SampleView::showRegisterFail(const std::string& reason) const {
    std::cout << "\n  " << ERR << "등록 실패: " << reason << RESET << "\n";
    pauseForInput();
}

void SampleView::showInvalidInput() const {
    std::cout << "\n  " << ERR << "잘못된 입력입니다. 메뉴 번호를 다시 입력하세요." << RESET << "\n";
    pauseForInput();
}

void SampleView::showEmpty() const {
    clearScreen();
    printSep();
    std::cout << "  " << SEP << "등록된 시료가 없습니다." << RESET << "\n";
    printSep();
    pauseForInput();
}
