#include "ProductionView.h"
#include "ConsoleColor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <string>

using namespace Color;

static void printSep() {
    std::cout << SEP << "-------------------------------------------" << RESET << "\n";
}

static std::time_t parseJobStartTime(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

static std::string formatJobTime(std::time_t t) {
    std::tm tm = {};
    localtime_s(&tm, &t);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

static std::string formatRemaining(double seconds) {
    int s = static_cast<int>(seconds);
    if (s <= 0) return "완료됨";
    int m = s / 60; s %= 60;
    int h = m / 60; m %= 60;
    if (h > 0)
        return std::to_string(h) + "시간 " + std::to_string(m) + "분 " + std::to_string(s) + "초";
    if (m > 0)
        return std::to_string(m) + "분 " + std::to_string(s) + "초";
    return std::to_string(s) + "초";
}

void ProductionView::showJobList(const std::vector<ProductionJob>& jobs,
                                  const std::vector<std::string>&   sampleNames) const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "생산라인 현황" << RESET << "\n";
    printSep();

    // 현재 생산 중 (jobs[0])
    const auto& cur     = jobs[0];
    std::string curName = sampleNames.empty() ? cur.sampleId : sampleNames[0];

    std::time_t startT   = parseJobStartTime(cur.startedAt);
    std::time_t endT     = startT + static_cast<std::time_t>(cur.totalDuration);
    std::time_t nowT     = std::time(nullptr);
    double      remaining = std::difftime(endT, nowT);

    std::cout << "  " << LABEL << "[현재 생산 중]" << RESET << "\n";
    std::cout << "  주문번호  : " << SECTION << cur.orderNumber << RESET << "\n";
    std::cout << "  시료명    : " << SECTION << curName << RESET
              << SEP << "  (" << cur.sampleId << ")" << RESET << "\n";
    std::cout << "  실 생산량 : " << SECTION << cur.actualQuantity << " ea" << RESET << "\n";
    std::cout << "  시작 시각 : " << SEP << cur.startedAt << RESET << "\n";
    std::cout << "  예상 완료 : " << SEP << formatJobTime(endT) << RESET
              << "   남은 시간: " << LABEL << formatRemaining(remaining) << RESET << "\n";
    printSep();

    // 대기 중 (jobs[1..])
    int waiting = static_cast<int>(jobs.size()) - 1;
    if (waiting > 0) {
        std::cout << "  " << SECTION << "[대기 중 — " << waiting << "건]" << RESET << "\n";
        for (int i = 1; i < static_cast<int>(jobs.size()); ++i) {
            const auto& j     = jobs[i];
            std::string jName = i < static_cast<int>(sampleNames.size()) ? sampleNames[i] : j.sampleId;
            std::time_t jStart = parseJobStartTime(j.startedAt);
            std::cout << "  " << LABEL << "[" << i << "]" << RESET << " "
                      << SECTION << j.orderNumber << RESET << "   "
                      << jName << "   " << j.actualQuantity << " ea"
                      << "   " << SEP << "예상 시작: " << formatJobTime(jStart) << RESET << "\n";
        }
        printSep();
    }

    pauseForInput();
}

void ProductionView::showEmpty() const {
    clearScreen();
    printSep();
    std::cout << "  " << SECTION << "생산라인 현황" << RESET << "\n";
    printSep();
    std::cout << "  현재 생산 대기 중인 작업이 없습니다.\n";
    printSep();
    pauseForInput();
}
