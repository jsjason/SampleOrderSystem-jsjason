# Phase 1 설계 — MVC 조립

## 개요

`SampleView`, `SampleController`, `AppController`, `MainView`를 설계하고,
`main.cpp` Release 분기에서 이들을 조립하는 방법을 정의한다.

---

## 파일 구성

```
SampleOrderSystem-jsjason/
├── main.cpp                            ← Release 분기 조립 완성
├── View/
│   ├── ConsoleColor.h                  ← 색상 상수 + clearScreen/pauseForInput 유틸리티
│   ├── MainView.h/.cpp
│   └── SampleView.h/.cpp
└── Controller/
    ├── AppController.h/.cpp
    └── SampleController.h/.cpp
```

---

## 의존성 구조

```
main()
  │
  ├─ SampleRepository  ──────────────────────────┐
  │                                              │
  ├─ SampleView                                  │
  ├─ SampleController(repo, view) ───────────────┤
  │                                              │
  ├─ MainView                                    │
  └─ AppController(repo, sampleCtrl, mainView) ──┘
         │
         └─ Run()  ← 메인 루프 시작
```

모든 의존성은 **생성자 참조 주입**. 소유권은 `main()`이 가진다.

---

## 콘솔 유틸리티 (`View/ConsoleColor.h`)

헤더 전용. 모든 View `.cpp` 파일이 `#include "ConsoleColor.h"` 하나로 색상과 화면 제어를 공유한다.

### 색상 팔레트

| 상수 | ANSI 코드 | 색상 | 적용 대상 |
|------|-----------|------|----------|
| `TITLE` | `\033[94m` | 진한 파란색 | 시스템 제목 |
| `SECTION` | `\033[97m` | 밝은 흰색 | 섹션 헤더, 강조 수치 |
| `LABEL` | `\033[96m` | 하늘색 | 메뉴 번호 `[N]`, 시료 ID 태그 |
| `SEP` | `\033[90m` | 옅은 회색 | `===`/`---` 구분자, 보조 텍스트, `(준비 중)` |
| `PROMPT` | `\033[93m` | 노란색 | 입력 프롬프트 (`선택 >`, `검색어 입력 >`) |
| `SUCCESS` | `\033[92m` | 초록색 | 등록 성공 등 긍정 피드백 |
| `ERR` | `\033[91m` | 빨간색 | 오류·실패 메시지 |
| `RESET` | `\033[0m` | — | 색상 초기화 |

### 화면 제어 유틸리티

```cpp
inline void clearScreen();      // \033[2J\033[H — 화면 지우고 커서 홈으로
inline void pauseForInput();    // "[ Enter 키를 누르면 계속합니다 ]" 출력 후 대기
```

> `clearScreen()`은 ANSI VT100 이스케이프 코드를 사용하므로, `main()` 진입부에서
> `SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)` 을 반드시 먼저 호출해야 한다.

### 화면 갱신 규칙

| 호출 지점 | 동작 |
|----------|------|
| `MainView::showMenu()` 진입 시 | `clearScreen()` |
| `SampleView::showMenu()` 진입 시 | `clearScreen()` |
| `SampleView::promptRegisterInput()` 진입 시 | `clearScreen()` |
| `SampleView::showList()` 출력 후 | `pauseForInput()` |
| `SampleView::showSearchResult()` 출력 후 | `pauseForInput()` |
| `SampleView::showEmpty()` 출력 후 | `pauseForInput()` |
| `SampleView::showRegisterSuccess/Fail()` 출력 후 | `pauseForInput()` |
| `MainView::showNotImplemented()` 출력 후 | `pauseForInput()` |
| `MainView::showInvalidInput()` 출력 후 | `pauseForInput()` |
| `SampleView::showInvalidInput()` 출력 후 | `pauseForInput()` |

Phase 2~4에서 추가되는 View 파일도 동일한 규칙을 따른다.

### `promptMenuChoice()` 구현 주의사항

`std::cin >> int`가 실패(문자 입력 등)할 경우 **C++ 표준은 변수를 `0`으로 설정**한다.
초기화 값(`-1`)은 유지되지 않으므로 `cin.clear()` 이후 반드시 명시적으로 복원해야 한다.
그렇지 않으면 `case 0: return;`이 실행되어 프로그램이 안내 없이 종료된다.

```cpp
int choice = -1;
if (!(std::cin >> choice)) {
    std::cin.clear();
    choice = -1;  // 추출 실패 시 C++11은 0으로 덮어쓰므로 명시적으로 복원
}
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
return choice;
```

이 패턴을 모든 `promptMenuChoice()` 구현체에 동일하게 적용한다.

---

## `MainView`

### 역할

메인 메뉴 출력, 메뉴 번호 입력 수집.
Phase 1에서는 시료 수/총 재고만 표시하며, [2]~[6]은 "준비 중"으로 표시한다.

### 헤더 선언 (`View/MainView.h`)

```cpp
#pragma once

class MainView {
public:
    void showMenu(int sampleCount, int totalStock) const;
    int  promptMenuChoice() const;
    void showNotImplemented() const;
    void showInvalidInput() const;
};
```

### 화면 레이아웃

> 진입 시 `clearScreen()` 호출.

```
===========================================
  S-Semi 시료 생산주문관리 시스템
  2026-06-12 15:30:00
  시료: 3개    총 재고: 980 ea
===========================================
  [1] 시료 관리
  [2] 시료 주문            (준비 중)
  [3] 주문 승인/거절       (준비 중)
  [4] 모니터링             (준비 중)
  [5] 생산라인 조회        (준비 중)
  [6] 출고 처리            (준비 중)
  [0] 종료
===========================================
선택 > _
```

> Phase 4에서 주문 수/생산 대기 항목이 추가되고, [2]~[6] "준비 중" 표시가 제거된다.

---

## `SampleView`

### 역할

시료 관리 서브메뉴 출력, 등록 입력 수집, 목록/검색 결과 출력.

### 헤더 선언 (`View/SampleView.h`)

```cpp
#pragma once
#include <string>
#include <vector>
#include "../Model/Sample.h"

class SampleView {
public:
    void showMenu() const;
    int  promptMenuChoice() const;

    Sample      promptRegisterInput() const;   // ID, 이름, 생산시간, 수율, 초기재고 입력
    std::string promptSearchKeyword() const;

    void showList(const std::vector<Sample>& samples) const;
    void showSearchResult(const std::vector<Sample>& samples,
                          const std::string& keyword) const;
    void showRegisterSuccess(const Sample& s) const;
    void showRegisterFail(const std::string& reason) const;
    void showEmpty() const;
    void showInvalidInput() const;
};
```

### 화면 레이아웃

**서브메뉴**

> 진입 시 `clearScreen()` 호출.

```
-------------------------------------------
  시료 관리
-------------------------------------------
  [1] 시료 등록
  [2] 시료 목록
  [3] 이름 검색
  [0] 돌아가기
-------------------------------------------
선택 > _
```

**시료 등록 입력**

> 진입 시 `clearScreen()` 호출. 결과 출력 후 `pauseForInput()`.

```
-------------------------------------------
  시료 등록
-------------------------------------------
시료 ID              : S-003
시료명               : GaN 에피택셜-4인치
평균 생산시간(min/ea) : 0.3
수율 (0.0~1.0)       : 0.78
초기 재고 (ea)       : 0

  시료가 등록되었습니다: [S-003] GaN 에피택셜-4인치

  [ Enter 키를 누르면 계속합니다 ]
```

**시료 목록**

> 진입 시 `clearScreen()` 호출. 출력 후 `pauseForInput()`.

```
-------------------------------------------
  등록 시료 목록  (2건)
-------------------------------------------
  [S-001]  실리콘 웨이퍼-8인치  |  0.50 min/ea  |  수율 0.92  |  재고 480 ea
  [S-002]  GaN 에피택셜-4인치   |  0.30 min/ea  |  수율 0.78  |  재고 0 ea
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**검색 결과**

> 진입 시 `clearScreen()` 호출. 출력 후 `pauseForInput()`.

```
-------------------------------------------
  검색 결과: "웨이퍼"  (1건)
-------------------------------------------
  [S-001]  실리콘 웨이퍼-8인치  |  0.50 min/ea  |  수율 0.92  |  재고 480 ea
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `SampleController`

### 역할

시료 관리 서브메뉴 루프. View로부터 입력을 받아 Repository를 호출하고 결과를 View에 전달한다.

### 헤더 선언 (`Controller/SampleController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../View/SampleView.h"

class SampleController {
public:
    SampleController(SampleRepository& repo, SampleView& view);
    void run();

private:
    void handleRegister();
    void handleList();
    void handleSearch();

    SampleRepository& repo_;
    SampleView&       view_;
};
```

### 흐름 설계

```
run()
  loop:
    view_.showMenu()
    choice = view_.promptMenuChoice()
    switch choice:
      1 → handleRegister()
      2 → handleList()
      3 → handleSearch()
      0 → return
      _ → view_.showInvalidInput()

handleRegister():
  sample = view_.promptRegisterInput()
  ok = repo_.add(sample)
  ok ? view_.showRegisterSuccess(sample)
     : view_.showRegisterFail("이미 등록된 시료 ID입니다.")

handleList():
  samples = repo_.getAll()
  samples.empty() ? view_.showEmpty()
                  : view_.showList(samples)

handleSearch():
  keyword = view_.promptSearchKeyword()
  results = repo_.searchByName(keyword)
  view_.showSearchResult(results, keyword)  // 0건이어도 결과 표시
```

---

## `AppController`

### 역할

최상위 메뉴 루프. Phase 1에서는 [1] 시료 관리만 동작하고 나머지는 "준비 중"을 표시한다.
Phase 2~4에서 생성자 인자와 switch 분기가 점진적으로 확장된다.

### 헤더 선언 (`Controller/AppController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../View/MainView.h"
#include "SampleController.h"

class AppController {
public:
    AppController(SampleRepository&   sampleRepo,
                  SampleController&   sampleCtrl,
                  MainView&           mainView);
    void run();

private:
    SampleRepository& sampleRepo_;
    SampleController& sampleCtrl_;
    MainView&         mainView_;
};
```

### 흐름 설계

```
run()
  loop:
    count = sampleRepo_.getAll().size()
    total = sum of stock in getAll()
    mainView_.showMenu(count, total)
    choice = mainView_.promptMenuChoice()
    switch choice:
      1 → sampleCtrl_.run()
      2~6 → mainView_.showNotImplemented()
      0 → return
      _ → mainView_.showInvalidInput()
```

---

## `main.cpp` — Release 분기 조립

```cpp
// ANSI 색상 코드 활성화 (Windows 10+, VT100)
HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD  mode = 0;
GetConsoleMode(hOut, &mode);
SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

// Repository 조립
SampleRepository  sampleRepo("data/samples.json");

SampleView        sampleView;
SampleController  sampleCtrl(sampleRepo, sampleView);

MainView          mainView;
AppController     app(sampleRepo, sampleCtrl, mainView);

app.run();
```

> - `data/` 디렉터리는 첫 번째 저장 시 `std::filesystem::create_directories()`로 자동 생성된다.
> - VT 처리 활성화는 `clearScreen()`과 색상 코드의 전제 조건이므로 `app.run()` 이전에 반드시 실행한다.

---

## Phase 1 완료 기준 체크리스트

- [x] Debug 빌드: `SampleRepository` 10개 테스트 모두 PASS (SKIP 0개)
- [x] Release 빌드: 메인 메뉴 정상 출력, [2]~[6] "준비 중" 표시
- [x] Release 빌드: [1] 시료 등록 → `data/samples.json` 생성 및 저장 확인
- [x] Release 빌드: 프로그램 재시작 후 등록한 시료 데이터 유지 확인
- [x] Release 빌드: 이름 검색 대소문자 무관 동작 확인
- [x] Release 빌드: 색상 팔레트 정상 출력 (제목 파란색, 구분자 회색, 메뉴 번호 하늘색 등)
- [x] Release 빌드: 메뉴 진입/결과 화면마다 이전 출력이 지워지고 refresh됨을 확인
- [x] Release 빌드: 범위 밖 숫자(예: 8, 9) 입력 시 "잘못된 입력" 안내 출력 확인
- [x] Release 빌드: 알파벳 문자(예: x) 입력 시 "잘못된 입력" 안내 출력 확인 (종료 없음)
