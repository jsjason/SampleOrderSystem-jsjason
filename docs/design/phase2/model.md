# Phase 2 설계 — Model 레이어

## 개요

`OrderStatus` enum, `Order` 구조체, `OrderRepository` 클래스를 설계한다.
주문번호 자동 생성 로직과 상태별 필터링이 핵심이다.
Model 레이어는 View/Controller를 참조하지 않는다.

---

## 파일 구성

```
SampleOrderSystem-jsjason/
└── Model/
    ├── Order.h     ← OrderStatus enum + Order 구조체 + OrderRepository 선언
    └── Order.cpp   ← 구현
```

---

## `OrderStatus` enum

```cpp
enum class OrderStatus {
    RESERVED,   // 주문 접수 (초기 상태)
    CONFIRMED,  // 승인 완료 — 출고 대기
    PRODUCING,  // 승인 완료 — 재고 부족으로 생산 중
    REJECTED,   // 거절
    RELEASED    // 출고 완료
};
```

### 문자열 변환 헬퍼 (익명 네임스페이스)

`toJson()`/`fromJson()` 구현에서 사용. `.cpp` 내부에 정의한다.

```cpp
static std::string statusToString(OrderStatus s) {
    switch (s) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::RELEASED:  return "RELEASED";
    }
    return "RESERVED";
}

static OrderStatus statusFromString(const std::string& s) {
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "RELEASED")  return OrderStatus::RELEASED;
    return OrderStatus::RESERVED;
}
```

---

## `Order` 구조체

### 헤더 선언 (`Model/Order.h`)

```cpp
#pragma once
#include <string>
#include "../json.h"

enum class OrderStatus { RESERVED, CONFIRMED, PRODUCING, REJECTED, RELEASED };

struct Order {
    std::string orderNumber;    // "ORD-YYYYMMDD-NNNN"
    std::string sampleId;
    std::string customerName;
    int         quantity;
    OrderStatus status;
    std::string createdAt;      // "YYYY-MM-DD HH:MM:SS"

    nlohmann::json toJson() const;
    static Order   fromJson(const nlohmann::json& j);
};
```

### JSON 필드 매핑

| 구조체 필드 | JSON 키 | 타입 |
|------------|---------|------|
| `orderNumber` | `"orderNumber"` | string |
| `sampleId` | `"sampleId"` | string |
| `customerName` | `"customerName"` | string |
| `quantity` | `"quantity"` | number (int) |
| `status` | `"status"` | string (`"RESERVED"` 등) |
| `createdAt` | `"createdAt"` | string |

### 구현 스케치 (`Model/Order.cpp`)

```cpp
nlohmann::json Order::toJson() const {
    return {
        {"orderNumber",  orderNumber},
        {"sampleId",     sampleId},
        {"customerName", customerName},
        {"quantity",     quantity},
        {"status",       statusToString(status)},
        {"createdAt",    createdAt}
    };
}

Order Order::fromJson(const nlohmann::json& j) {
    return {
        j.at("orderNumber").get<std::string>(),
        j.at("sampleId").get<std::string>(),
        j.at("customerName").get<std::string>(),
        j.at("quantity").get<int>(),
        statusFromString(j.at("status").get<std::string>()),
        j.at("createdAt").get<std::string>()
    };
}
```

---

## `OrderRepository` 클래스

### 헤더 선언 (`Model/Order.h` — 동일 파일에 선언)

```cpp
#include <vector>
#include <optional>

class OrderRepository {
public:
    // filePath가 빈 문자열("")이면 파일 I/O를 수행하지 않는다 (테스트용 in-memory 모드).
    explicit OrderRepository(const std::string& filePath);

    // 주문 접수. 주문번호·createdAt을 내부에서 자동 생성하여 저장 후 반환.
    Order add(const std::string& sampleId,
              const std::string& customerName,
              int quantity);

    // 전체 주문 목록 반환.
    std::vector<Order> getAll() const;

    // 주문번호로 조회. 없으면 std::nullopt.
    std::optional<Order> findByNumber(const std::string& orderNumber) const;

    // 특정 상태의 주문만 반환.
    std::vector<Order> filterByStatus(OrderStatus status) const;

    // 주문 상태 변경. 주문번호가 없으면 false.
    // Phase 3에서 승인/거절/생산완료 처리에 사용.
    bool updateStatus(const std::string& orderNumber, OrderStatus newStatus);

private:
    std::string        filePath_;
    std::vector<Order> orders_;
    std::string        lastDate_;   // 마지막 주문번호 생성 날짜 (YYYYMMDD)
    int                nextSeq_;    // 다음 시퀀스 번호

    void        load();
    void        save();
    std::string generateOrderNumber();   // 주문번호 자동 생성
    std::string currentDateString();     // 오늘 날짜를 "YYYYMMDD" 형식으로 반환
    std::string currentDateTimeString(); // 현재 일시를 "YYYY-MM-DD HH:MM:SS" 형식으로 반환
};
```

### 메서드 상세

#### `add()`

```
1. order.orderNumber = generateOrderNumber()
2. order.sampleId     = sampleId
3. order.customerName = customerName
4. order.quantity     = quantity
5. order.status       = OrderStatus::RESERVED
6. order.createdAt    = currentDateTimeString()
7. orders_.push_back(order)
8. save()
9. return order
```

#### `getAll()`

```
orders_ 사본 반환
```

#### `findByNumber()`

```
orders_ 순회 → orderNumber 일치 항목 반환
없으면 std::nullopt
```

#### `filterByStatus()`

```
orders_ 순회 → status 일치 항목만 모아 반환
```

#### `updateStatus()`

```
orders_ 순회 → orderNumber 일치 원소 직접 수정
  일치: order.status = newStatus, save(), return true
  없음: return false
```

### 주문번호 생성 로직

형식: `ORD-YYYYMMDD-NNNN`

시퀀스 번호는 **날짜별로 1부터 시작**하는 4자리 0-패딩 정수다.

**초기화 (생성자 호출 시)**:

```
1. lastDate_ = currentDateString()
2. orders_ 중 "ORD-{lastDate_}-" 로 시작하는 주문의 시퀀스 번호 추출
3. nextSeq_ = 추출된 최댓값 + 1 (없으면 1)
```

**generateOrderNumber()**:

```
today = currentDateString()
if today != lastDate_:
    lastDate_ = today
    nextSeq_  = 1

orderNumber = "ORD-" + today + "-" + zero_pad(nextSeq_, 4)
nextSeq_++
return orderNumber
```

시퀀스 번호 파싱 예시:
```
"ORD-20260612-0043" → lastSlash 이후 "0043" → stoi → 43
```

### 영속성 설계

```
생성자
  └─ filePath_가 비어있지 않고 파일이 존재하면
       JSON 배열 파싱 → orders_ 에 로드
       → nextSeq_ 초기화

save()
  └─ filePath_가 비어있지 않으면
       orders_ 전체를 JSON 배열로 직렬화 → 파일 덮어쓰기
```

JSON 파일 형식 (`data/orders.json`):

```json
[
  {
    "orderNumber": "ORD-20260612-0001",
    "sampleId": "S-001",
    "customerName": "삼성전자 파운드리",
    "quantity": 100,
    "status": "RESERVED",
    "createdAt": "2026-06-12 15:30:00"
  }
]
```

---

## 단위 테스트 구현 지침

테스트 파일: `Tests/OrderRepositoryTest.cpp`

### in-memory 모드 사용

```cpp
OrderRepository repo("");  // filePath = "" → 파일 I/O 없음
```

각 `TEST()` 내부에서 `repo`를 로컬로 생성하면 테스트 간 상태 공유가 없다.

### 픽스처 패턴

```cpp
// repo에 주문 한 건 추가하고 반환된 Order를 반환하는 헬퍼
static Order placeOrder(OrderRepository& repo,
                        const std::string& sampleId = "S-001",
                        const std::string& customer = "테스트 고객",
                        int quantity = 100) {
    return repo.add(sampleId, customer, quantity);
}
```

### Phase 2 테스트 구현 힌트

| 테스트 | 검증 포인트 |
|--------|------------|
| `주문을_접수하면_RESERVED_상태로_저장된다` | `add()` 반환값의 `status == RESERVED`, `findByNumber()` 확인 |
| `주문번호는_ORD_날짜_시퀀스_형식으로_생성된다` | `orderNumber`가 `"ORD-"` + 8자리 숫자 + `"-"` + 4자리 숫자 형식인지 확인. `std::regex` 또는 문자열 접두사 검사 사용 |
| `전체_주문_목록을_반환한다` | 2건 추가 후 `getAll().size() == 2` |
| `주문번호로_특정_주문을_조회한다` | `findByNumber()` 반환값 필드 확인 |
| `RESERVED_상태_주문만_필터링한다` | RESERVED 2건 추가, `filterByStatus(RESERVED).size() == 2` |
| `CONFIRMED_상태_주문만_필터링한다` | `updateStatus()` 로 상태 변경 후 `filterByStatus(CONFIRMED)` 확인 |

> `주문번호는_ORD_날짜_시퀀스_형식으로_생성된다` 테스트는 실제 날짜에 의존하므로
> 정규식(`ORD-\d{8}-\d{4}`) 또는 `orderNumber.substr(0, 4) == "ORD-"` 등의 형식 검사로 작성한다.
