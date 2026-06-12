# Phase 2 설계 — MVC 조립

## 개요

`OrderView`, `OrderController`를 신규 작성하고,
`AppController` / `main.cpp`에서 [2] 시료 주문 메뉴를 연결한다.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── View/
│   └── OrderView.h/.cpp        ← 신규
├── Controller/
│   └── OrderController.h/.cpp  ← 신규
│   └── AppController.h/.cpp    ← 수정 (OrderController 추가)
└── main.cpp                    ← 수정 (OrderRepository + OrderController 조립)
```

---

## 의존성 구조 (Phase 2 이후)

```
main()
  │
  ├─ SampleRepository  ──────────────────────────────────┐
  ├─ OrderRepository   ──────────────────────────────┐   │
  │                                                  │   │
  ├─ SampleView                                      │   │
  ├─ SampleController(sampleRepo, sampleView) ───────┤   │
  │                                                  │   │
  ├─ OrderView                                       │   │
  ├─ OrderController(sampleRepo, orderRepo, orderView)┘  │
  │                                                      │
  ├─ MainView                                            │
  └─ AppController(sampleRepo, sampleCtrl,               │
                   orderCtrl, mainView) ─────────────────┘
         │
         └─ run()
```

`OrderController`는 시료 ID 유효성 검사를 위해 `SampleRepository`를 참조한다.

---

## `OrderView`

### 역할

주문 접수 서브메뉴 출력, 주문 입력 수집, 주문 목록 출력.

### 헤더 선언 (`View/OrderView.h`)

```cpp
#pragma once
#include <string>
#include <vector>
#include "../Model/Order.h"

class OrderView {
public:
    void showMenu() const;
    int  promptMenuChoice() const;

    struct OrderInput {
        std::string sampleId;
        std::string customerName;
        int         quantity;
    };
    // 시료 ID만 먼저 수집 (clearScreen + 폼 헤더 포함).
    std::string promptSampleId() const;
    // 시료 ID 검증 통과 후 나머지 항목 수집 (화면 유지, clearScreen 없음).
    OrderInput  promptOrderInput(const std::string& sampleId) const;

    void showOrderSuccess(const Order& o) const;
    void showOrderFail(const std::string& reason) const;
    void showList(const std::vector<Order>& orders) const;
    void showEmpty() const;
    void showInvalidInput() const;
};
```

> `promptMenuChoice()`는 Phase 1과 동일한 C++11 주의사항을 적용한다.
> (`cin >> int` 실패 시 `choice = 0` 덮어쓰기 → `choice = -1` 명시 복원.
> 자세한 내용은 [Phase 1 mvc.md](../phase1/mvc.md) "promptMenuChoice() 구현 주의사항" 참조.)

### 화면 레이아웃

**서브메뉴**

> 진입 시 `clearScreen()` 호출.

```
-------------------------------------------
  시료 주문
-------------------------------------------
  [1] 주문 접수
  [2] 주문 목록
  [0] 돌아가기
-------------------------------------------
선택 > _
```

**주문 접수 입력 — 1단계: 시료 ID (`promptSampleId()`)**

> 진입 시 `clearScreen()` 호출. 시료 ID만 수집 후 즉시 컨트롤러에서 검증.

```
-------------------------------------------
  주문 접수
-------------------------------------------
시료 ID        : _
```

**주문 접수 입력 — 2단계: 나머지 항목 (`promptOrderInput(sampleId)`)**

> 1단계 화면 유지 (clearScreen 없음). 시료 ID 검증 통과 후에만 진입.

```
-------------------------------------------
  주문 접수
-------------------------------------------
시료 ID        : S-001
고객명          : _
주문 수량 (ea) : _
```

**주문 접수 성공**

> 입력 화면 아래에 결과 출력 후 `pauseForInput()`.

```
  주문이 접수되었습니다.
  주문번호 : ORD-20260612-0001
  시료 ID  : S-001
  고객명    : 삼성전자 파운드리
  수량      : 100 ea
  상태      : RESERVED
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**주문 접수 실패**

> 입력 화면 아래에 오류 출력 후 `pauseForInput()`. 시료 ID 오류 시 고객명·수량은 묻지 않음.

```
  오류: 등록되지 않은 시료 ID입니다: S-999
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**주문 목록**

> 진입 시 `clearScreen()` 호출. 출력 후 `pauseForInput()`.

```
-------------------------------------------
  주문 목록  (2건)
-------------------------------------------
  [ORD-20260612-0001]  S-001  삼성전자 파운드리
    수량: 100 ea  |  RESERVED  |  2026-06-12 15:30:00
  [ORD-20260612-0002]  S-002  SK하이닉스
    수량:  50 ea  |  RESERVED  |  2026-06-12 15:45:00
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `OrderController`

### 역할

주문 접수 서브메뉴 루프. 시료 ID 유효성 검사 후 주문을 생성한다.

### 헤더 선언 (`Controller/OrderController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../View/OrderView.h"

class OrderController {
public:
    OrderController(SampleRepository& sampleRepo,
                    OrderRepository&  orderRepo,
                    OrderView&        view);
    void run();

private:
    void handlePlaceOrder();
    void handleListOrders();

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    OrderView&        view_;
};
```

### 흐름 설계

```
run()
  loop:
    view_.showMenu()
    choice = view_.promptMenuChoice()
    switch choice:
      1 → handlePlaceOrder()
      2 → handleListOrders()
      0 → return
      _ → view_.showInvalidInput()

handlePlaceOrder():
  sampleId = view_.promptSampleId()
  if !sampleRepo_.findById(sampleId).has_value():
      view_.showOrderFail("등록되지 않은 시료 ID입니다: " + sampleId)
      return                          ← 고객명·수량은 묻지 않음
  input = view_.promptOrderInput(sampleId)
  if input.quantity <= 0:
      view_.showOrderFail("수량은 1 이상의 정수여야 합니다.")
      return
  order = orderRepo_.add(input.sampleId, input.customerName, input.quantity)
  view_.showOrderSuccess(order)

handleListOrders():
  orders = orderRepo_.getAll()
  orders.empty() ? view_.showEmpty()
                 : view_.showList(orders)
```

---

## `AppController` 수정

### 헤더 선언 변경 (`Controller/AppController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../View/MainView.h"
#include "SampleController.h"
#include "OrderController.h"

class AppController {
public:
    AppController(SampleRepository& sampleRepo,
                  SampleController& sampleCtrl,
                  OrderController&  orderCtrl,
                  MainView&         mainView);
    void run();

private:
    SampleRepository& sampleRepo_;
    SampleController& sampleCtrl_;
    OrderController&  orderCtrl_;
    MainView&         mainView_;
};
```

### 흐름 설계 변경

```
run()
  loop:
    count = sampleRepo_.getAll().size()
    total = sum of stock in getAll()
    mainView_.showMenu(count, total)
    choice = mainView_.promptMenuChoice()
    switch choice:
      1 → sampleCtrl_.run()
      2 → orderCtrl_.run()          ← Phase 2에서 연결 (기존 showNotImplemented 대체)
      3, 4, 5, 6 → mainView_.showNotImplemented()
      0 → return
      _ → mainView_.showInvalidInput()
```

---

## `main.cpp` — Release 분기 수정

```cpp
SampleRepository  sampleRepo("data/samples.json");
OrderRepository   orderRepo("data/orders.json");

SampleView        sampleView;
SampleController  sampleCtrl(sampleRepo, sampleView);

OrderView         orderView;
OrderController   orderCtrl(sampleRepo, orderRepo, orderView);

MainView          mainView;
AppController     app(sampleRepo, sampleCtrl, orderCtrl, mainView);

app.run();
```

---

## 화면 갱신 규칙 (Phase 2 추가분)

| 호출 지점 | 동작 |
|----------|------|
| `OrderView::showMenu()` 진입 시 | `clearScreen()` |
| `OrderView::promptSampleId()` 진입 시 | `clearScreen()` |
| `OrderView::promptOrderInput(sampleId)` 진입 시 | clearScreen 없음 — 1단계 화면 유지 |
| `OrderView::showOrderSuccess/Fail()` 출력 후 | `pauseForInput()` |
| `OrderView::showList()` 출력 후 | `pauseForInput()` |
| `OrderView::showEmpty()` 출력 후 | `pauseForInput()` |
| `OrderView::showInvalidInput()` 출력 후 | `pauseForInput()` |

---

## Phase 2 완료 기준 체크리스트

- [x] Debug 빌드: `OrderRepository` 6개 테스트 모두 PASS (누적 16 / 43)
- [x] Release 빌드: 메인 메뉴 [2] 시료 주문 진입 가능 확인
- [x] Release 빌드: 존재하는 시료 ID로 주문 접수 → 주문번호 `ORD-YYYYMMDD-NNNN` 형식 확인
- [x] Release 빌드: 존재하지 않는 시료 ID 입력 시 고객명·수량을 묻지 않고 즉시 오류 출력 확인
- [x] Release 빌드: 수량에 0, 음수, 문자열 입력 시 오류 출력 확인 (1 이상 정수만 허용)
- [x] Release 빌드: 주문 목록에서 접수한 주문 표시 확인
- [x] Release 빌드: 프로그램 재시작 후 주문 데이터 유지 확인 (`data/orders.json`)
