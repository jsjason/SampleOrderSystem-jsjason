# Phase 3a 설계 — MVC 조립

## 개요

주문 승인/거절 기능을 구현한다.
`OrderView`에 승인/거절 화면 메서드를 추가하고,
`OrderController`에 승인/거절 로직을 추가한 뒤,
`AppController`에서 [3] 주문 승인/거절 메뉴를 연결한다.

승인/거절의 핵심 비즈니스 로직은 View와 분리된 자유 함수(`applyApproval`, `applyRejection`)로
구현하여 View 없이 단위 테스트할 수 있게 한다.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── Model/
│   └── ProductionQueue.h/.cpp      ← 신규 (Phase 3a model.md 참조)
├── View/
│   └── OrderView.h/.cpp            ← 수정 (승인/거절 화면 메서드 추가)
├── Controller/
│   ├── OrderController.h/.cpp      ← 수정 (ProductionQueue 참조 추가, 승인/거절 로직)
│   └── AppController.h/.cpp        ← 수정 ([3] 메뉴 연결)
└── main.cpp                        ← 수정 (ProductionQueue 조립)
```

---

## 의존성 구조 (Phase 3a 이후)

```
main()
  │
  ├─ SampleRepository   ─────────────────────────────────────┐
  ├─ OrderRepository    ─────────────────────────────────────┤
  ├─ ProductionQueue    ─────────────────────────────────────┤
  │                                                          │
  ├─ SampleView                                              │
  ├─ SampleController(sampleRepo, sampleView)                │
  │                                                          │
  ├─ OrderView                                               │
  ├─ OrderController(sampleRepo, orderRepo,                  │
  │                  prodQueue, orderView) ──────────────────┤
  │                                                          │
  ├─ MainView                                                │
  └─ AppController(sampleRepo, sampleCtrl,                   │
                   orderCtrl, mainView) ────────────────────┘
```

`OrderController`가 `ProductionQueue`를 추가로 참조한다.

---

## 비즈니스 로직 분리 — `applyApproval()` / `applyRejection()`

승인/거절 핵심 로직을 자유 함수로 분리한다.
`OrderController.h`에 선언하고 `OrderController.cpp`에 구현한다.
`OrderApprovalTest.cpp`에서 이 함수를 직접 호출하여 View 없이 테스트한다.

### `ApprovalResult` 구조체

```cpp
struct ApprovalResult {
    OrderStatus newStatus;      // CONFIRMED 또는 PRODUCING
    int         shortage;       // 0이면 재고 충분
    int         actualQuantity; // shortage > 0일 때 실 생산량
};
```

### 함수 선언 (`Controller/OrderController.h`)

```cpp
// 승인 처리. 재고 판단 후 상태 전환 및 생산 큐 등록.
ApprovalResult applyApproval(const std::string& orderNumber,
                              SampleRepository&  sampleRepo,
                              OrderRepository&   orderRepo,
                              ProductionQueue&   prodQueue);

// 거절 처리. 상태를 REJECTED로 전환.
bool applyRejection(const std::string& orderNumber,
                    OrderRepository&   orderRepo);
```

### `applyApproval()` 흐름

```
order  = orderRepo.findByNumber(orderNumber).value()
sample = sampleRepo.findById(order.sampleId).value()

if sample.stock >= order.quantity:
    sampleRepo.deductStock(order.sampleId, order.quantity)
    orderRepo.updateStatus(orderNumber, CONFIRMED)
    return { CONFIRMED, 0, 0 }
else:
    shortage  = order.quantity - sample.stock
    actualQty = calculateActualQuantity(shortage, sample.yield)
    prodQueue.enqueue(orderNumber, order.sampleId, actualQty)
    orderRepo.updateStatus(orderNumber, PRODUCING)
    return { PRODUCING, shortage, actualQty }
```

### `applyRejection()` 흐름

```
return orderRepo.updateStatus(orderNumber, REJECTED)
```

---

## `OrderView` 수정

### 헤더 선언 변경 (`View/OrderView.h`)

```cpp
#pragma once
#include <string>
#include <vector>
#include "../Model/Order.h"
#include "../Controller/OrderController.h"  // ApprovalResult

class OrderView {
public:
    // --- Phase 2 (기존) ---
    void showMenu() const;
    int  promptMenuChoice() const;

    struct OrderInput { std::string sampleId, customerName; int quantity; };
    std::string promptSampleId() const;
    OrderInput  promptOrderInput(const std::string& sampleId) const;

    void showOrderSuccess(const Order& o) const;
    void showOrderFail(const std::string& reason) const;
    void showList(const std::vector<Order>& orders) const;
    void showEmpty() const;
    void showInvalidInput() const;

    // --- Phase 3a 신규 ---
    // 승인 대기(RESERVED) 주문 목록. 번호(1~N) 포함.
    void showReservedList(const std::vector<Order>& orders) const;
    // 처리할 주문 번호 선택 (1~N, 0=돌아가기).
    int  promptOrderSelection(int count) const;
    // 승인(1) / 거절(2) / 취소(0) 선택. 선택 전 대상 주문 요약 출력.
    int  promptApprovalDecision(const Order& order) const;
    // 승인 결과: 재고 충분 → CONFIRMED.
    void showApprovalConfirmed(const Order& order) const;
    // 승인 결과: 재고 부족 → PRODUCING. 부족분과 실 생산량 표시.
    void showApprovalProducing(const Order& order, int shortage, int actualQty) const;
    // 거절 결과.
    void showRejected(const Order& order) const;
    // RESERVED 주문이 없을 때.
    void showNoReservedOrders() const;
};
```

> `OrderController.h`를 포함하므로, `OrderController.h`는 `OrderView.h`를 포함하면
> 순환 참조가 발생한다. 이를 피하기 위해 `ApprovalResult` 구조체를 `Model/Order.h`
> 하단에 정의하고, 양쪽이 `Order.h`만 포함하도록 한다.

### 화면 레이아웃

**승인 대기 주문 목록 (`showReservedList()`)**

> 진입 시 `clearScreen()` 호출. 출력 후 `promptOrderSelection()` 호출.

```
-------------------------------------------
  주문 승인/거절  (2건)
-------------------------------------------
  [1] ORD-20260612-0001  S-001  100 ea
      고객명: 삼성전자 파운드리  |  2026-06-12 15:30:00
  [2] ORD-20260612-0002  S-002   50 ea
      고객명: SK하이닉스         |  2026-06-12 15:45:00
-------------------------------------------
선택 (0=돌아가기) > _
```

**승인/거절 결정 (`promptApprovalDecision()`)**

> 이전 화면 유지 (clearScreen 없음). 선택 주문 요약 후 결정 프롬프트.

```
선택 (0=돌아가기) > 1

  대상 주문: ORD-20260612-0001  S-001  100 ea  삼성전자 파운드리
-------------------------------------------
  [1] 승인
  [2] 거절
  [0] 취소
-------------------------------------------
선택 > _
```

**승인 결과 — CONFIRMED (`showApprovalConfirmed()`)**

> 결과 출력 후 `pauseForInput()`.

```
  승인 완료: 재고가 확인되어 주문이 확정되었습니다.
  주문번호 : ORD-20260612-0001
  상태      : CONFIRMED
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**승인 결과 — PRODUCING (`showApprovalProducing()`)**

> 결과 출력 후 `pauseForInput()`.

```
  승인 완료: 재고 부족으로 생산이 등록되었습니다.
  주문번호 : ORD-20260612-0001
  상태      : PRODUCING
  부족분    :  120 ea  →  실 생산량: 145 ea
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**거절 결과 (`showRejected()`)**

> 결과 출력 후 `pauseForInput()`.

```
  거절 완료: 주문이 거절 처리되었습니다.
  주문번호 : ORD-20260612-0001
  상태      : REJECTED
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

**승인 대기 없음 (`showNoReservedOrders()`)**

> 출력 후 `pauseForInput()`.

```
-------------------------------------------
  승인 대기 중인 주문이 없습니다.
-------------------------------------------
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `OrderController` 수정

### 헤더 선언 변경 (`Controller/OrderController.h`)

```cpp
#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/OrderView.h"

struct ApprovalResult {
    OrderStatus newStatus;
    int         shortage;
    int         actualQuantity;
};

ApprovalResult applyApproval(const std::string& orderNumber,
                              SampleRepository&  sampleRepo,
                              OrderRepository&   orderRepo,
                              ProductionQueue&   prodQueue);

bool applyRejection(const std::string& orderNumber,
                    OrderRepository&   orderRepo);

class OrderController {
public:
    OrderController(SampleRepository& sampleRepo,
                    OrderRepository&  orderRepo,
                    ProductionQueue&  prodQueue,
                    OrderView&        view);
    void run();           // [2] 시료 주문 서브메뉴 (Phase 2, 변경 없음)
    void runApproval();   // [3] 주문 승인/거절 진입점 (Phase 3a 신규)

private:
    void handlePlaceOrder();
    void handleListOrders();
    void handleApprove(const Order& order);
    void handleReject(const Order& order);

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    OrderView&        view_;
};
```

### 흐름 설계

```
runApproval()
  orders = orderRepo_.filterByStatus(RESERVED)
  if orders.empty():
      view_.showNoReservedOrders()
      return
  view_.showReservedList(orders)
  idx = view_.promptOrderSelection(orders.size())
  if idx == 0: return
  if idx < 1 || idx > (int)orders.size():
      view_.showInvalidInput(); return

  order    = orders[idx - 1]
  decision = view_.promptApprovalDecision(order)
  switch decision:
    1 → handleApprove(order)
    2 → handleReject(order)
    0 → return
    _ → view_.showInvalidInput()

handleApprove(order):
  result = applyApproval(order.orderNumber, sampleRepo_, orderRepo_, prodQueue_)
  if result.newStatus == CONFIRMED:
      view_.showApprovalConfirmed(order)
  else:
      view_.showApprovalProducing(order, result.shortage, result.actualQuantity)

handleReject(order):
  applyRejection(order.orderNumber, orderRepo_)
  view_.showRejected(order)
```

---

## `AppController` 수정

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
      2 → orderCtrl_.run()
      3 → orderCtrl_.runApproval()   ← Phase 3a에서 연결 (기존 showNotImplemented 대체)
      4, 5, 6 → mainView_.showNotImplemented()
      0 → return
      _ → mainView_.showInvalidInput()
```

헤더/생성자는 변경 없다 (`OrderController` 참조가 이미 있으며, `ProductionQueue`는
`OrderController` 생성자로 전달되므로 `AppController`가 직접 참조할 필요가 없다).

---

## `main.cpp` — Release 분기 수정

```cpp
SampleRepository  sampleRepo("data/samples.json");
OrderRepository   orderRepo("data/orders.json");
ProductionQueue   prodQueue("data/production.json");

SampleView        sampleView;
SampleController  sampleCtrl(sampleRepo, sampleView);

OrderView         orderView;
OrderController   orderCtrl(sampleRepo, orderRepo, prodQueue, orderView);

MainView          mainView;
AppController     app(sampleRepo, sampleCtrl, orderCtrl, mainView);

app.run();
```

---

## 화면 갱신 규칙 (Phase 3a 추가분)

| 호출 지점 | 동작 |
|----------|------|
| `OrderView::showReservedList()` 진입 시 | `clearScreen()` |
| `OrderView::promptApprovalDecision()` 진입 시 | clearScreen 없음 — 목록 화면 유지 |
| `OrderView::showApprovalConfirmed/Producing()` 출력 후 | `pauseForInput()` |
| `OrderView::showRejected()` 출력 후 | `pauseForInput()` |
| `OrderView::showNoReservedOrders()` 출력 후 | `pauseForInput()` |

---

## 단위 테스트 구현 지침

테스트 파일: `Tests/OrderApprovalTest.cpp`

### 픽스처 패턴

```cpp
#include "../Controller/OrderController.h"

class OrderApproval : public ::testing::Test {
protected:
    SampleRepository sampleRepo{""};
    OrderRepository  orderRepo{""};
    ProductionQueue  prodQueue{""};

    // stock ea의 재고를 가진 시료 등록
    void addSample(const std::string& id, int stock, double yield = 0.92) {
        sampleRepo.add(Sample{id, "테스트 시료", 0.5, yield, stock});
    }

    // 주문 접수 후 주문번호 반환
    std::string placeOrder(const std::string& sampleId, int qty) {
        return orderRepo.add(sampleId, "테스트 고객", qty).orderNumber;
    }
};
```

### Phase 3a 테스트 구현 힌트

| 테스트 | 검증 포인트 |
|--------|------------|
| `재고가_충분하면_승인시_CONFIRMED로_전환된다` | `applyApproval()` 결과 `newStatus == CONFIRMED`, `findByNumber()` 상태 확인 |
| `재고가_충분하면_승인시_재고가_주문량만큼_차감된다` | `applyApproval()` 후 `sampleRepo.findById()->stock` 감소 확인 |
| `재고가_충분하면_승인시_생산_큐에_등록되지_않는다` | `applyApproval()` 후 `prodQueue.empty() == true` |
| `재고가_부족하면_승인시_PRODUCING으로_전환된다` | `result.newStatus == PRODUCING` |
| `재고가_부족하면_승인시_생산_큐에_자동_등록된다` | `applyApproval()` 후 `prodQueue.empty() == false` |
| `재고가_부족하면_부족분만큼_생산_작업이_생성된다` | `prodQueue.front()->actualQuantity` == `ceil(shortage / (yield × 0.9))` 확인 |
| `재고가_0이면_주문량_전체가_생산_대상이다` | 재고 0인 시료로 `applyApproval()`, `shortage == order.quantity` 확인 |
| `거절하면_REJECTED로_전환된다` | `applyRejection()` 후 상태 확인 |
| `거절해도_재고는_변경되지_않는다` | `applyRejection()` 전후 `stock` 동일 확인 |

---

## Phase 3a 완료 기준 체크리스트

- [ ] Debug 빌드: `OrderRepository` 상태 전환 5개 테스트 PASS
- [ ] Debug 빌드: `ProductionQueue` 기본 5개 테스트 PASS
- [ ] Debug 빌드: `OrderApproval` 9개 테스트 PASS (누적 35 / 46)
- [ ] Release 빌드: 메인 메뉴 [3] 주문 승인/거절 진입 가능 확인
- [ ] Release 빌드: 재고 충분 시 승인 → CONFIRMED 전환, 재고 차감 확인
- [ ] Release 빌드: 재고 부족 시 승인 → PRODUCING 전환, 생산 큐 등록 확인
- [ ] Release 빌드: 거절 → REJECTED 전환, 재고 변동 없음 확인
- [ ] Release 빌드: 프로그램 재시작 후 생산 큐 데이터 유지 확인 (`data/production.json`)
