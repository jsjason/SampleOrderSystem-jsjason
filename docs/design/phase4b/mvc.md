# Phase 4b 설계 — MVC 조립

## 개요

`MonitorController` / `MonitorView`를 신규 작성하고,
`MainView` 현황 요약과 `AppController`를 완성하여 전체 시스템을 마무리한다.
새로 추가되는 단위 테스트는 없으며, UI 전용 기능이다.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── View/
│   ├── MonitorView.h/.cpp           ← 신규
│   └── MainView.h/.cpp              ← 수정 (showMenu 시그니처 변경)
├── Controller/
│   ├── MonitorController.h/.cpp     ← 신규
│   └── AppController.h/.cpp         ← 수정 (MonitorController 멤버 추가, [4] 연결)
└── main.cpp                         ← 수정 (MonitorController 조립)
```

---

## 재고 상태(StockLevel) 판단 로직

모니터링 화면에서 각 시료의 재고 상태를 세 단계로 표기한다.

| 상태 | 조건 |
|------|------|
| **고갈** | `sample.stock == 0` |
| **부족** | `stock > 0` AND RESERVED 주문 대기 수량 > 가용 재고 |
| **여유** | 그 외 |

### 가용 재고 계산 (`availableStock`)

Phase 4a에서 확립한 reserved 계산과 동일한 방식을 사용한다.
CONFIRMED 주문은 아직 출고되지 않아 재고에 잡혀 있으므로 선점으로 간주한다.

```
reservedQty     = Σ PRODUCING 주문 수량(해당 시료)
                + Σ CONFIRMED 주문 수량(해당 시료)
availableStock  = max(0, sample.stock - reservedQty)
pendingReserved = Σ RESERVED  주문 수량(해당 시료)

고갈: sample.stock == 0
부족: stock > 0 AND pendingReserved > availableStock
여유: 그 외
```

`pendingReserved`가 0이면 RESERVED 주문이 없어 항상 **여유**로 분류된다.

### 데이터 구조

```cpp
enum class StockLevel { AMPLE, SHORT, EMPTY };

struct SampleStockStatus {
    std::string sampleId;
    std::string sampleName;
    int         stock;
    StockLevel  level;
};
```

---

## `MonitorController` — 신규

### 헤더 (`Controller/MonitorController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/MonitorView.h"

class MonitorController {
public:
    MonitorController(SampleRepository& sampleRepo,
                      OrderRepository&  orderRepo,
                      ProductionQueue&  prodQueue,
                      MonitorView&      view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    MonitorView&      view_;
};
```

### 흐름 (`run()`)

```
run():
    prodQueue_.processCompleted(sampleRepo_, orderRepo_)   // 진입 시 완료 체크

    orders  = orderRepo_.getAll()
    samples = sampleRepo_.getAll()

    // 주문 현황 집계 (REJECTED 제외)
    OrderStats stats = { 0, 0, 0, 0 }
    for order in orders:
        switch order.status:
            RESERVED  → stats.reserved++
            CONFIRMED → stats.confirmed++
            PRODUCING → stats.producing++
            RELEASED  → stats.released++

    // 시료별 재고 상태 판단
    stockStatuses = []
    for sample in samples:
        reservedQty = 0
        for job in prodQueue_.getAll():
            if job.sampleId == sample.id:
                o = orderRepo_.findByNumber(job.orderNumber)
                if o.has_value(): reservedQty += o->quantity
        for o in orderRepo_.filterByStatus(CONFIRMED):
            if o.sampleId == sample.id: reservedQty += o.quantity

        availableStock = max(0, sample.stock - reservedQty)

        pendingReserved = 0
        for o in orderRepo_.filterByStatus(RESERVED):
            if o.sampleId == sample.id: pendingReserved += o.quantity

        if sample.stock == 0:
            level = EMPTY
        elif pendingReserved > availableStock:
            level = SHORT
        else:
            level = AMPLE

        stockStatuses.push_back({ sample.id, sample.name, sample.stock, level })

    view_.showDashboard(stats, stockStatuses)
```

### 집계 구조체

```cpp
struct OrderStats {
    int reserved  = 0;
    int confirmed = 0;
    int producing = 0;
    int released  = 0;
};
```

`OrderStats`는 `MonitorView.h`에 함께 선언한다.
`SampleStockStatus`도 마찬가지로 `MonitorView.h`에 선언한다.

---

## `MonitorView` — 신규

### 헤더 (`View/MonitorView.h`)

```cpp
#pragma once
#include <vector>
#include <string>

enum class StockLevel { AMPLE, SHORT, EMPTY };

struct SampleStockStatus {
    std::string sampleId;
    std::string sampleName;
    int         stock;
    StockLevel  level;
};

struct OrderStats {
    int reserved  = 0;
    int confirmed = 0;
    int producing = 0;
    int released  = 0;
};

class MonitorView {
public:
    void showDashboard(const OrderStats&                   stats,
                       const std::vector<SampleStockStatus>& stockStatuses) const;
};
```

### 화면 레이아웃 (`showDashboard()`)

```
===========================================
  모니터링 대시보드
===========================================
  [주문 현황]
  RESERVED   :   3건
  CONFIRMED  :   2건
  PRODUCING  :   1건
  RELEASED   :   5건
  -------------------------------------------
  합계        :  11건
===========================================
  [재고 현황]
  시료명                  재고        상태
  실리콘 웨이퍼-8인치      480 ea    [여유]
  GaN 에피택셜-4인치         0 ea    [고갈]
  GaAs 웨이퍼-6인치         30 ea    [부족]
===========================================
  [ Enter 키를 누르면 계속합니다 ]
```

#### 재고 상태 표기 규칙

| `StockLevel` | 표기 | 색상 |
|-------------|------|------|
| `AMPLE`     | `[여유]` | `SUCCESS` (초록) |
| `SHORT`     | `[부족]` | `ERR` (빨강) |
| `EMPTY`     | `[고갈]` | `SEP` (회색) |

#### 시료 목록이 비어 있을 때

```
===========================================
  모니터링 대시보드
===========================================
  [주문 현황]
  RESERVED   :   0건
  CONFIRMED  :   0건
  PRODUCING  :   0건
  RELEASED   :   0건
  -------------------------------------------
  합계        :   0건
===========================================
  [재고 현황]
  등록된 시료가 없습니다.
===========================================
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `MainView` 수정

### 시그니처 변경

메인 메뉴 현황 요약에 전체 주문 수와 생산라인 대기 수를 추가한다.

```cpp
// 기존
void showMenu(int sampleCount, int totalStock) const;

// 변경 후
void showMenu(int sampleCount, int totalStock,
              int orderCount,  int queueCount) const;
```

### 현황 요약 출력 변경

```cpp
// 기존 출력
std::cout << "  시료: " << SECTION << sampleCount << "개" << RESET
          << "    총 재고: " << SECTION << totalStock << " ea" << RESET << "\n";

// 변경 후 출력 (2줄로 나눔)
std::cout << "  시료: " << SECTION << sampleCount << "개" << RESET
          << "    총 재고: " << SECTION << totalStock << " ea" << RESET
          << "    주문: "   << SECTION << orderCount << "건" << RESET
          << "    생산 대기: " << SECTION << queueCount << "건" << RESET << "\n";
```

### [4] 메뉴 항목 `(준비 중)` 제거

```cpp
// 기존
std::cout << "  " << LABEL << "[4]" << RESET << " 모니터링             " << SEP << "(준비 중)" << RESET << "\n";

// 변경 후
std::cout << "  " << LABEL << "[4]" << RESET << " 모니터링\n";
```

---

## `AppController` 수정

### 헤더 (`AppController.h`) — `MonitorController` 멤버 추가

```cpp
#include "MonitorController.h"   // Phase 4b 추가

class AppController {
public:
    AppController(SampleRepository&     sampleRepo,
                  OrderRepository&      orderRepo,
                  ProductionQueue&      prodQueue,
                  SampleController&     sampleCtrl,
                  OrderController&      orderCtrl,
                  ProductionController& productionCtrl,
                  ReleaseController&    releaseCtrl,
                  MonitorController&    monitorCtrl,    // Phase 4b 추가
                  MainView&             mainView);
    void run();

private:
    // ... 기존 멤버 ...
    MonitorController& monitorCtrl_;   // Phase 4b 추가
};
```

### `run()` 수정

현황 데이터를 수집하여 `showMenu()`에 전달하고, [4] 메뉴를 연결한다.

```cpp
void AppController::run() {
    while (true) {
        prodQueue_.processCompleted(sampleRepo_, orderRepo_);

        const auto samples = sampleRepo_.getAll();
        int sampleCount = static_cast<int>(samples.size());
        int totalStock  = 0;
        for (const auto& s : samples) totalStock += s.stock;

        int orderCount = static_cast<int>(orderRepo_.getAll().size());   // Phase 4b 추가
        int queueCount = static_cast<int>(prodQueue_.getAll().size());   // Phase 4b 추가

        mainView_.showMenu(sampleCount, totalStock, orderCount, queueCount);
        int choice = mainView_.promptMenuChoice();

        switch (choice) {
            case 1: sampleCtrl_.run();        break;
            case 2: orderCtrl_.run();         break;
            case 3: orderCtrl_.runApproval(); break;
            case 4: monitorCtrl_.run();       break;  // Phase 4b: 연결
            case 5: productionCtrl_.run();    break;
            case 6: releaseCtrl_.run();       break;
            case 0: return;
            default: mainView_.showInvalidInput(); break;
        }
    }
}
```

`showNotImplemented()` 분기는 완전히 제거된다. `showNotImplemented()`와 `showInvalidInput()`은 모두 사용 가능하지만, Phase 4b 완료 시 `showNotImplemented()`의 호출 지점은 없어진다.

---

## `main.cpp` — Release 분기 수정

```cpp
ReleaseView          releaseView;
ReleaseController    releaseCtrl(sampleRepo, orderRepo, releaseView);

MonitorView          monitorView;                                        // Phase 4b 추가
MonitorController    monitorCtrl(sampleRepo, orderRepo, prodQueue,       // Phase 4b 추가
                                 monitorView);

MainView             mainView;
AppController        app(sampleRepo, orderRepo, prodQueue,
                         sampleCtrl, orderCtrl, productionCtrl,
                         releaseCtrl, monitorCtrl,                       // Phase 4b 추가
                         mainView);
```

---

## Phase 4b 완료 기준 체크리스트

### 단위 테스트 (Debug 빌드)

- [ ] 전체 48개 테스트 PASS, 0개 SKIPPED (기존 그대로 유지)

### 수동 테스트 (Release 빌드)

**메인 현황 요약**
- [ ] 메인 메뉴 상단에 시료 수, 총 재고, 전체 주문 수, 생산 대기 수 모두 표시
- [ ] 주문/생산 상태 변경 후 메인 복귀 시 수치가 즉시 갱신됨

**모니터링 대시보드**
- [ ] [4] 모니터링 진입 → RESERVED/CONFIRMED/PRODUCING/RELEASED 건수 표시
- [ ] REJECTED 주문은 집계에서 제외됨
- [ ] 시료별 재고 상태(여유/부족/고갈) 표기 확인
- [ ] 재고 0인 시료 → `[고갈]` (회색) 표시
- [ ] RESERVED 주문 합계 > 가용 재고인 시료 → `[부족]` (빨강) 표시
- [ ] 그 외 → `[여유]` (초록) 표시
- [ ] Enter 후 메인 메뉴로 복귀 확인
