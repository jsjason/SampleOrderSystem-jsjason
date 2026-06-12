#pragma once
#include <iostream>
#include <limits>

namespace Color {
    constexpr const char* TITLE   = "\033[94m";  // 진한 파란색  — 시스템 제목
    constexpr const char* SECTION = "\033[97m";  // 밝은 흰색    — 섹션 헤더
    constexpr const char* LABEL   = "\033[96m";  // 하늘색       — 메뉴 번호, ID 태그
    constexpr const char* SEP     = "\033[90m";  // 옅은 회색    — 구분자, 보조 텍스트
    constexpr const char* PROMPT  = "\033[93m";  // 노란색       — 입력 프롬프트
    constexpr const char* SUCCESS = "\033[92m";  // 초록색       — 성공 메시지
    constexpr const char* ERR     = "\033[91m";  // 빨간색       — 오류 메시지
    constexpr const char* RESET   = "\033[0m";
}

inline void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

inline void pauseForInput() {
    std::cout << Color::SEP << "\n  [ Enter 키를 누르면 계속합니다 ]" << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
