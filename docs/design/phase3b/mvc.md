# Phase 3b 설계 — MVC 조립

## 개요

`ProductionController` / `ProductionView`를 신규 작성하고,
`processCompleted()`를 재고 확인이 필요한 모든 진입 시점에 연결한다.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── Model/
│   └── ProductionQueue.h/.cpp      ← 수정 (enqueue 시그니처, processCompleted 추가)
├── View/
│   └── ProductionView.h/.cpp       ← 신규
├── Controller/
│   ├── ProductionController.h/.cpp ← 신규
│   ├── AppController.h/.cpp        ← 수정 (processCompleted 호출, [5] 메뉴 연결)
│   └── OrderController.h/.cpp      ← 수정 (runApproval 진입 시 processCompleted 호출)
└── main.cpp                        ← 수정 (ProductionController 조립)
```

---

## `processCompleted()` 호출 지점

`prodQueue_.processCompleted(sampleRepo_, orderRepo_)`를 다음 지점에 삽입한다.
(`now`는 기본값 사용 — 실제 시스템 시간 기준으로 동작.)

| 호출 위치 | 이유 |
|----------|------|
| `AppController::run()` 루프 **상단** | 모든 화면 이동 전 공통 적용 |
| `OrderController::runApproval()` **진입 직후** | 승인 화면에서 시료 재고를 직접 확인 |
| `ProductionController::run()` 루프 **상단** | 생산라인 화면에서 큐 상태 표시 |
| `MonitorController::run()` 루프 **상단** | Phase 4 구현 시 추가 |

---

## `ProductionController` — 신규

### 헤더 (`Controller/ProductionController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/ProductionView.h"

class ProductionController {
public:
    ProductionController(SampleRepository&  sampleRepo,
                         OrderRepository&   orderRepo,
                         ProductionQueue&   prodQueue,
                         ProductionView&    view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    ProductionView&   view_;
};
```

### 흐름 (`run()`)

```
run():
    prodQueue_.processCompleted(sampleRepo_, orderRepo_)   // 진입 시 완료 체크
    jobs = prodQueue_.getAll()
    if jobs.empty():
        view_.showEmpty()
        return
    // sampleNames 빌드 (showJobList에서 시료명 표시용)
    sampleNames = []
    for job in jobs:
        s = sampleRepo_.findById(job.sampleId)
        sampleNames.push_back(s.has_value() ? s->name : job.sampleId)
    view_.showJobList(jobs, sampleNames)
    pauseForInput()
```

---

## `ProductionView` — 신규

### 헤더 (`View/ProductionView.h`)

```cpp
#pragma once
#include <vector>
#include <string>
#include "../Model/ProductionQueue.h"

class ProductionView {
public:
    void showJobList(const std::vector<ProductionJob>& jobs,
                     const std::vector<std::string>&   sampleNames) const;
    void showEmpty() const;
};
```

### 화면 레이아웃

**`showJobList()`** — `pauseForInput()` 포함

jobs[0]이 현재 생산 중, jobs[1] 이후는 대기 중으로 표시한다.
`예상 완료` / `예상 시작`은 `startedAt + totalDuration`을 `formatDateTime()`으로 변환한다.
현재 생산 중인 작업의 남은 시간은 `totalDuration - difftime(now, startedAt)`으로 계산한다.

```
-------------------------------------------
  생산라인 현황
-------------------------------------------
  [현재 생산 중]
  주문번호  : ORD-20260612-0001
  시료명    : GaAs 웨이퍼  (S-001)
  실 생산량 : 206 ea
  시작 시각 : 2026-06-12 15:30:00
  예상 완료 : 2026-06-12 15:32:04   남은 시간: 1분 23초
-------------------------------------------
  [대기 중 — 1건]
  [1] ORD-20260612-0002   실리콘 웨이퍼   100 ea   예상 시작: 2026-06-12 15:32:04
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**`showEmpty()`** — `pauseForInput()` 포함

```
-------------------------------------------
  생산라인 현황
-------------------------------------------
  현재 생산 대기 중인 작업이 없습니다.
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `AppController` 수정

### `run()` 루프 상단에 `processCompleted()` 추가

```cpp
void AppController::run() {
    while (true) {
        prodQueue_.processCompleted(sampleRepo_, orderRepo_);  // ← Phase 3b 추가
        // ... showMenu, promptMenuChoice, switch ...
        case 5: productionCtrl_.run(); break;
        // ...
    }
}
```

### 헤더 / 생성자 변경

`ProductionController`를 멤버로 추가한다.
`prodQueue_`도 직접 참조해야 하므로 생성자에 추가한다.

```cpp
class AppController {
public:
    AppController(SampleRepository&     sampleRepo,
                  ProductionQueue&      prodQueue,         // Phase 3b 추가
                  SampleController&     sampleCtrl,
                  OrderController&      orderCtrl,
                  ProductionController& productionCtrl,    // Phase 3b 추가
                  MainView&             mainView);
    void run();
private:
    SampleRepository&     sampleRepo_;
    ProductionQueue&      prodQueue_;                      // Phase 3b 추가
    OrderRepository&      orderRepo_;                      // processCompleted 인자
    SampleController&     sampleCtrl_;
    OrderController&      orderCtrl_;
    ProductionController& productionCtrl_;                 // Phase 3b 추가
    MainView&             mainView_;
};
```

> `AppController`가 `OrderRepository`도 참조해야 `processCompleted()`를 호출할 수 있다.
> 생성자 파라미터에 추가하거나, `prodQueue_`의 `processCompleted` 호출을
> 각 서브컨트롤러로 위임하는 방식도 가능하다.
> **권장**: `AppController` 생성자에 `OrderRepository&`를 추가. `main.cpp`에서 주입.

---

## `OrderController` 수정

`runApproval()` 진입 직후 `processCompleted()`를 호출한다.
`OrderController`는 이미 `sampleRepo_`, `orderRepo_`, `prodQueue_`를 보유하고 있으므로
추가 의존성 없이 호출 가능하다.

```cpp
void OrderController::runApproval() {
    prodQueue_.processCompleted(sampleRepo_, orderRepo_);  // ← Phase 3b 추가
    auto orders = orderRepo_.filterByStatus(OrderStatus::RESERVED);
    // ... 이하 기존 로직 동일
}
```

---

## `applyApproval()` 수정

`enqueue()` 시그니처 변경에 따라 `avgProductionTime`을 추가로 전달한다.

```cpp
// Phase 3a
prodQueue.enqueue(orderNumber, order.sampleId, actualQty);

// Phase 3b (변경 후)
prodQueue.enqueue(orderNumber, order.sampleId, actualQty, sample.avgProductionTime);
```

`applyApproval()`은 이미 `sample` 객체를 보유하고 있으므로 변경이 최소화된다.

---

## `main.cpp` — Release 분기 수정

```cpp
SampleRepository    sampleRepo("data/samples.json");
OrderRepository     orderRepo("data/orders.json");
ProductionQueue     prodQueue("data/production.json");

SampleView          sampleView;
SampleController    sampleCtrl(sampleRepo, sampleView);

OrderView           orderView;
OrderController     orderCtrl(sampleRepo, orderRepo, prodQueue, orderView);

ProductionView      productionView;
ProductionController productionCtrl(sampleRepo, orderRepo, prodQueue, productionView);

MainView            mainView;
AppController       app(sampleRepo, orderRepo, prodQueue,
                        sampleCtrl, orderCtrl, productionCtrl, mainView);
app.Run();
```

---

## 화면 갱신 규칙 (Phase 3b 추가분)

| 호출 지점 | 동작 |
|----------|------|
| `ProductionView::showJobList()` 진입 시 | `clearScreen()` |
| `ProductionView::showEmpty()` 진입 시 | `clearScreen()` |
| `ProductionView::showJobList/Empty()` 출력 후 | `pauseForInput()` |

---

## Phase 3b 완료 기준 체크리스트

- [ ] Debug 빌드: `ProductionCalculation` 4개 테스트 PASS
- [ ] Debug 빌드: `ProductionQueue` 자동 완료 3개 테스트 PASS
- [ ] Debug 빌드: `OrderApproval` 생산 완료 3개 테스트 PASS (누적 45 / 46)
- [ ] Release 빌드: `avgProductionTime=0.01` 시료로 6초 내 자동 완료 확인
- [ ] Release 빌드: 복수 작업 순차 완료 (두 번째 작업 startedAt = 첫 번째 완료 시각) 확인
- [ ] Release 빌드: 프로그램 재시작 후 진행 중 작업 상태 유지 확인 (`data/production.json`)
