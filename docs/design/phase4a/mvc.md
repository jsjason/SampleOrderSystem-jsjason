# Phase 4a 설계 — MVC 조립

## 개요

`ReleaseController` / `ReleaseView`를 신규 작성하고 `AppController`에 [6] 출고 처리 메뉴를 연결한다.
재고 차감 시점 수정(`stock-deduction-fix.md`)으로 인해 `OrderController`와 `ProductionQueue`도 함께 변경된다.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── Model/
│   └── ProductionQueue.cpp          ← processCompleted(): deductStock 제거
├── View/
│   └── ReleaseView.h/.cpp           ← 신규
├── Controller/
│   ├── ReleaseController.h/.cpp     ← 신규
│   ├── OrderController.cpp          ← applyApproval(): deductStock 제거, reserved 계산 변경
│   │                                   runApproval(): reserved 계산 변경
│   └── AppController.h/.cpp         ← ReleaseController 멤버 추가, [6] 메뉴 연결
└── main.cpp                         ← ReleaseController 조립
```

---

## `ProductionQueue` 수정 — `processCompleted()`

완료 처리 블록에서 `deductStock` 호출을 제거한다.

```cpp
// 기존 완료 처리
auto order = orderRepo.findByNumber(jobs_.front().orderNumber);
if (order.has_value()) {
    sampleRepo.deductStock(jobs_.front().sampleId, order->quantity);  // ← 제거
    orderRepo.updateStatus(jobs_.front().orderNumber, OrderStatus::CONFIRMED);
}

// 수정 후
auto order = orderRepo.findByNumber(jobs_.front().orderNumber);
if (order.has_value()) {
    orderRepo.updateStatus(jobs_.front().orderNumber, OrderStatus::CONFIRMED);
}
```

---

## `OrderController` 수정

### `applyApproval()` 자유 함수

재고 충분 분기에서 `deductStock`을 제거하고, reserved 계산에 CONFIRMED 수량을 추가한다.

```cpp
ApprovalResult applyApproval(const std::string& orderNumber,
                              SampleRepository&  sampleRepo,
                              OrderRepository&   orderRepo,
                              ProductionQueue&   prodQueue) {
    auto order  = orderRepo.findByNumber(orderNumber).value();
    auto sample = sampleRepo.findById(order.sampleId).value();

    // PRODUCING 선점 수량
    int reserved = 0;
    for (const auto& job : prodQueue.getAll()) {
        auto o = orderRepo.findByNumber(job.orderNumber);
        if (o.has_value()) reserved += o->quantity;
    }
    // CONFIRMED 선점 수량 (출고 전, 재고에 아직 잡혀 있음) ← Phase 4a 추가
    for (const auto& o : orderRepo.filterByStatus(OrderStatus::CONFIRMED))
        reserved += o.quantity;

    int availableStock = std::max(0, sample.stock - reserved);

    if (availableStock >= order.quantity) {
        // deductStock 제거 ← Phase 4a 변경
        orderRepo.updateStatus(orderNumber, OrderStatus::CONFIRMED);
        return { OrderStatus::CONFIRMED, 0, 0 };
    }

    int shortage  = order.quantity - availableStock;
    int actualQty = calculateActualQuantity(shortage, sample.yield);
    prodQueue.enqueue(orderNumber, order.sampleId, actualQty, sample.avgProductionTime);
    orderRepo.updateStatus(orderNumber, OrderStatus::PRODUCING);
    return { OrderStatus::PRODUCING, shortage, actualQty };
}
```

### `runApproval()` — 표시용 reserved 계산 동기화

`runApproval()`의 `availableStock` 표시 로직도 동일하게 수정한다.

```cpp
void OrderController::runApproval() {
    prodQueue_.processCompleted(sampleRepo_, orderRepo_);
    // ... (RESERVED 목록 조회 및 표시)

    // PRODUCING 선점
    int reserved = 0;
    for (const auto& job : prodQueue_.getAll()) {
        auto pending = orderRepo_.findByNumber(job.orderNumber);
        if (pending.has_value()) reserved += pending->quantity;
    }
    // CONFIRMED 선점 ← Phase 4a 추가
    for (const auto& o : orderRepo_.filterByStatus(OrderStatus::CONFIRMED))
        reserved += o.quantity;

    int availableStock = std::max(0, sample.stock - reserved);
    int decision = view_.promptApprovalDecision(order, sample, availableStock);
    // ...
}
```

---

## `ReleaseController` — 신규

### 헤더 (`Controller/ReleaseController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../View/ReleaseView.h"

class ReleaseController {
public:
    ReleaseController(SampleRepository& sampleRepo,
                      OrderRepository&  orderRepo,
                      ReleaseView&      view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ReleaseView&      view_;

    void handleRelease(const Order& order);
};
```

### 흐름 (`run()`)

```
run():
    confirmedOrders = orderRepo_.filterByStatus(CONFIRMED)
    if confirmedOrders.empty():
        view_.showEmpty()
        return

    sampleNames = []
    for order in confirmedOrders:
        s = sampleRepo_.findById(order.sampleId)
        sampleNames.push_back(s.has_value() ? s->name : order.sampleId)

    view_.showConfirmedList(confirmedOrders, sampleNames)
    idx = view_.promptOrderSelection(confirmedOrders.size())

    if idx == 0: return
    if idx < 1 or idx > confirmedOrders.size():
        view_.showInvalidInput()
        return

    handleRelease(confirmedOrders[idx - 1])
```

### `handleRelease()`

```cpp
void ReleaseController::handleRelease(const Order& order) {
    sampleRepo_.deductStock(order.sampleId, order.quantity);  // 출고 시점 재고 차감
    orderRepo_.updateStatus(order.orderNumber, OrderStatus::RELEASED);
    view_.showReleaseSuccess(order);
}
```

---

## `ReleaseView` — 신규

### 헤더 (`View/ReleaseView.h`)

```cpp
#pragma once
#include <vector>
#include <string>
#include "../Model/Order.h"

class ReleaseView {
public:
    void showConfirmedList(const std::vector<Order>&        orders,
                           const std::vector<std::string>& sampleNames) const;
    void showEmpty()                              const;
    void showReleaseSuccess(const Order& order)   const;
    void showInvalidInput()                       const;
    int  promptOrderSelection(int count)          const;
};
```

### 화면 레이아웃

**`showConfirmedList()`**

```
-------------------------------------------
  출고 처리 — 출고 대기 목록
-------------------------------------------
  번호  주문번호              시료명             고객명               수량
  [1]   ORD-20260612-0001   실리콘 웨이퍼      삼성전자 파운드리       100 ea
  [2]   ORD-20260612-0003   GaAs 웨이퍼       TSMC                  200 ea
-------------------------------------------
  [0] 돌아가기
주문 번호 선택:
```

**`showEmpty()`** — `pauseForInput()` 포함

```
-------------------------------------------
  출고 처리
-------------------------------------------
  출고 대기 중인 주문이 없습니다.
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**`showReleaseSuccess()`** — `pauseForInput()` 포함

```
-------------------------------------------
  출고 완료
-------------------------------------------
  주문번호 : ORD-20260612-0001
  시료명   : 실리콘 웨이퍼
  고객명   : 삼성전자 파운드리
  수량     : 100 ea
  상태     : CONFIRMED → RELEASED
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

`showReleaseSuccess()`가 출력할 시료명은 `ReleaseController::run()`에서 이미 구한
`sampleNames[idx-1]`을 전달하거나, View가 `SampleRepository`를 직접 참조하지 않도록
시료명 파라미터를 추가로 받는 방식 중 하나를 선택한다.
다른 View의 패턴과 일치하도록 **시료명을 파라미터로 전달**하는 방식을 권장한다.

```cpp
// 권장 시그니처
void showReleaseSuccess(const Order& order, const std::string& sampleName) const;
```

---

## `AppController` 수정

### 헤더 (`AppController.h`) — `ReleaseController` 멤버 추가

```cpp
#pragma once
#include "SampleController.h"
#include "OrderController.h"
#include "ProductionController.h"
#include "ReleaseController.h"   // Phase 4a 추가
#include "../View/MainView.h"
// (Model 헤더는 이미 포함)

class AppController {
public:
    AppController(SampleRepository&     sampleRepo,
                  OrderRepository&      orderRepo,
                  ProductionQueue&      prodQueue,
                  SampleController&     sampleCtrl,
                  OrderController&      orderCtrl,
                  ProductionController& productionCtrl,
                  ReleaseController&    releaseCtrl,      // Phase 4a 추가
                  MainView&             mainView);
    void run();

private:
    SampleRepository&     sampleRepo_;
    OrderRepository&      orderRepo_;
    ProductionQueue&      prodQueue_;
    SampleController&     sampleCtrl_;
    OrderController&      orderCtrl_;
    ProductionController& productionCtrl_;
    ReleaseController&    releaseCtrl_;                   // Phase 4a 추가
    MainView&             mainView_;
};
```

### `run()` — [6] 메뉴 연결

```cpp
switch (choice) {
    case 1: sampleCtrl_.run();        break;
    case 2: orderCtrl_.run();         break;
    case 3: orderCtrl_.runApproval(); break;
    case 5: productionCtrl_.run();    break;
    case 6: releaseCtrl_.run();       break;   // Phase 4a: 연결
    case 4:
        mainView_.showNotImplemented(); break; // Phase 4b에서 구현
    case 0: return;
    default: mainView_.showInvalidInput(); break;
}
```

---

## `main.cpp` — Release 분기 수정

```cpp
SampleRepository     sampleRepo("data/samples.json");
OrderRepository      orderRepo("data/orders.json");
ProductionQueue      prodQueue("data/production.json");

SampleView           sampleView;
SampleController     sampleCtrl(sampleRepo, orderRepo, prodQueue, sampleView);

OrderView            orderView;
OrderController      orderCtrl(sampleRepo, orderRepo, prodQueue, orderView);

ProductionView       productionView;
ProductionController productionCtrl(sampleRepo, orderRepo, prodQueue, productionView);

ReleaseView          releaseView;                                       // Phase 4a 추가
ReleaseController    releaseCtrl(sampleRepo, orderRepo, releaseView);   // Phase 4a 추가

MainView             mainView;
AppController        app(sampleRepo, orderRepo, prodQueue,
                         sampleCtrl, orderCtrl, productionCtrl,
                         releaseCtrl,                                   // Phase 4a 추가
                         mainView);
app.Run();
```

---

## Phase 4a 완료 기준 체크리스트

### 단위 테스트 (Debug 빌드)

- [ ] `재고가_충분하면_승인시_재고가_주문량만큼_차감된다` — 기댓값 500으로 수정 후 PASS
- [ ] `생산_완료시_시료_재고가_실생산량만큼_증가한다` — 기댓값 `actualQuantity`로 수정 후 PASS
- [ ] `생산_완료시_재고에서_주문량만큼_차감된다` — 기댓값 `actQty`(변화 없음)로 수정 후 PASS
- [ ] `CONFIRMED_주문을_출고하면_RELEASED로_전환된다` — 스킵 해제 후 구현, PASS
- [ ] 전체 48개 테스트 PASS, 0개 SKIPPED

### 수동 테스트 (Release 빌드)

- [ ] [6] 출고 처리 진입 → CONFIRMED 목록 표시 확인
- [ ] 특정 주문 선택 → 상태 CONFIRMED → RELEASED 전환 확인
- [ ] 출고 후 해당 시료 재고 감소 확인 (시료 관리에서 재고 조회)
- [ ] CONFIRMED 주문 없을 때 → "출고 대기 중인 주문이 없습니다" 출력 확인
- [ ] [0] 돌아가기 → 메인 메뉴 복귀 확인
