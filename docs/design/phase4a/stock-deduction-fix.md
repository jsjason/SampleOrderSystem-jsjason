# Phase 4a 설계 — 재고 차감 시점 수정

## 버그 개요

Phase 3a/3b에서 구현된 재고 차감 로직에 타이밍 오류가 있다.
`CONFIRMED` 전환 시점에 `deductStock(order.quantity)`가 호출되어
**출고 전에 재고가 먼저 줄어드는** 문제가 존재한다.

재고는 물리적으로 아이템이 출고될 때(`CONFIRMED → RELEASED`)에 감소해야 한다.

---

## 현재 동작 및 문제

### 재고 충분 경로 — `applyApproval()`

```cpp
// 현재 (잘못된 위치)
sampleRepo.deductStock(order.sampleId, order.quantity);   // ← 승인 시점 차감
orderRepo.updateStatus(orderNumber, OrderStatus::CONFIRMED);
```

주문 승인(`RESERVED → CONFIRMED`) 시점에 재고를 차감한다.
아이템은 아직 출하되지 않았는데 재고가 감소한다.

### 생산 경로 — `processCompleted()`

```cpp
// 현재 (잘못된 위치)
// 생산 중 incremental addStock() 으로 stock 이 actualQuantity 만큼 누적된 상태
sampleRepo.deductStock(jobs_.front().sampleId, order->quantity);  // ← 완료 시점 차감
orderRepo.updateStatus(jobs_.front().orderNumber, OrderStatus::CONFIRMED);
```

생산 완료(`PRODUCING → CONFIRMED`) 시점에 재고를 차감한다.
출고 대기 상태(CONFIRMED)인데도 재고가 이미 줄어 있다.

---

## 수정 방향

**모든 재고 차감을 출고 처리 시점(`CONFIRMED → RELEASED`)으로 단일화한다.**

| 위치 | 기존 | 변경 후 |
|------|------|--------|
| `applyApproval()` — 재고 충분 분기 | `deductStock` 호출 | 제거 |
| `processCompleted()` — 완료 처리 | `deductStock` 호출 | 제거 |
| `ReleaseController::handleRelease()` | (없음) | `deductStock` 추가 |

---

## 가용 재고 계산 변경

재고 차감이 출고 시점으로 이전되면, CONFIRMED 주문의 수량도
'아직 출고되지 않아 재고에 잡혀 있는 선점 물량'으로 취급해야 한다.
그렇지 않으면 CONFIRMED 주문이 대기 중인 재고를 새 주문이 가져가
출고 시 재고 부족이 발생할 수 있다.

```cpp
// 기존 reserved 계산 (PRODUCING 주문만)
int reserved = 0;
for (const auto& job : prodQueue.getAll()) {
    auto o = orderRepo.findByNumber(job.orderNumber);
    if (o.has_value()) reserved += o->quantity;
}

// 수정 후 (PRODUCING + CONFIRMED 모두 포함)
int reserved = 0;
for (const auto& job : prodQueue.getAll()) {
    auto o = orderRepo.findByNumber(job.orderNumber);
    if (o.has_value()) reserved += o->quantity;           // PRODUCING
}
for (const auto& o : orderRepo.filterByStatus(OrderStatus::CONFIRMED))
    reserved += o.quantity;                               // CONFIRMED (미출고)

int availableStock = std::max(0, sample.stock - reserved);
```

이 변경은 `applyApproval()` 자유 함수와 `OrderController::runApproval()` 표시용 계산
양쪽에 동일하게 적용한다.

---

## 영향받는 단위 테스트

기댓값이 달라지는 기존 테스트 3개를 수정한다. 테스트 총 개수(48개)는 유지된다.

### 수정 대상

#### `재고가_충분하면_승인시_재고가_주문량만큼_차감된다`

승인 후 재고가 변하지 않는 동작으로 기댓값 변경.

```cpp
// 수정 전
applyApproval(num, sampleRepo, orderRepo, prodQueue);
EXPECT_EQ(sampleRepo.findById("S-001")->stock, 400);  // 500 - 100

// 수정 후
applyApproval(num, sampleRepo, orderRepo, prodQueue);
EXPECT_EQ(sampleRepo.findById("S-001")->stock, 500);  // 변화 없음
```

#### `생산_완료시_시료_재고가_실생산량만큼_증가한다`

생산 완료 시 `deductStock` 제거로, 최종 재고 = `actualQuantity`(추가분 전부).

```cpp
// 수정 전
// addStock(actualQty) 후 deductStock(5) → 최종 재고 = actualQty - 5
EXPECT_EQ(sampleRepo.findById("S-001")->stock, result.actualQuantity - 5);

// 수정 후
// addStock(actualQty) 만 수행, deductStock 없음 → 최종 재고 = actualQty
EXPECT_EQ(sampleRepo.findById("S-001")->stock, result.actualQuantity);
```

#### `생산_완료시_재고에서_주문량만큼_차감된다`

생산 완료 시 차감 없음으로 기댓값 역전.

```cpp
// 수정 전
EXPECT_LT(sampleRepo.findById("S-001")->stock, actQty);  // stock < actQty (차감됨)

// 수정 후
EXPECT_EQ(sampleRepo.findById("S-001")->stock, actQty);  // stock == actQty (차감 없음)
```

### 신규 구현 테스트

`CONFIRMED_주문을_출고하면_RELEASED로_전환된다` — 스킵 해제 후 구현.

```cpp
TEST_F(OrderApproval, CONFIRMED_주문을_출고하면_RELEASED로_전환된다) {
    addSample("S-001", 500);
    auto num = placeOrder("S-001", 100);

    // 승인 후 재고 변화 없음 (수정된 동작)
    applyApproval(num, sampleRepo, orderRepo, prodQueue);
    ASSERT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::CONFIRMED);
    ASSERT_EQ(sampleRepo.findById("S-001")->stock, 500);

    // 출고 처리 — 재고 차감 + 상태 전환
    sampleRepo.deductStock("S-001", 100);
    orderRepo.updateStatus(num, OrderStatus::RELEASED);

    EXPECT_EQ(orderRepo.findByNumber(num)->status, OrderStatus::RELEASED);
    EXPECT_EQ(sampleRepo.findById("S-001")->stock, 400);
}
```

실제 구현에서는 `ReleaseController::handleRelease()`가 위 두 호출을 수행한다.
테스트는 `applyRelease()` 자유 함수를 별도로 추출하거나, 위 코드처럼 직접 호출해도 무방하다.
