# Phase 3a 설계 — Model 레이어

## 개요

`ProductionJob` 구조체와 `ProductionQueue` 클래스를 신규 작성한다.
Phase 3a에서는 기본 FIFO 큐 기능만 구현한다.
시간 기반 자동 완료 필드(`startedAt`, `totalDuration`)는 선언만 하고 실제 로직은 Phase 3b에서 추가한다.

기존 `OrderRepository.updateStatus()`는 Phase 2에서 이미 구현되어 있으며,
Phase 3a에서 승인/거절/PRODUCING 상태 전환에 처음 활용된다.

---

## 파일 구성

```
SampleOrderSystem-jsjason/
└── Model/
    ├── ProductionQueue.h   ← ProductionJob 구조체 + ProductionQueue 선언
    └── ProductionQueue.cpp ← 구현
```

---

## 실 생산량 계산

승인 시 재고가 부족한 경우 생산해야 할 수량:

```
실 생산량 = ceil(부족분 / (수율 × 0.9))
```

- `수율 × 0.9`: 생산 중 발생하는 추가 불량률을 감안한 보정
- `부족분 = 주문수량 - 현재재고` (현재재고가 0이면 주문수량 전체)
- `ceil`로 올림하여 정수 생산량 산출

구현 (`.cpp` 내 정적 헬퍼):

```cpp
// <cmath> 필요
static int calculateActualQuantity(int shortage, double yield) {
    return static_cast<int>(std::ceil(shortage / (yield * 0.9)));
}
```

Phase 3b의 `totalDuration` 계산에서도 이 함수를 재사용한다.

---

## `ProductionJob` 구조체

### 헤더 선언 (`Model/ProductionQueue.h`)

```cpp
#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../json.h"

struct ProductionJob {
    std::string orderNumber;    // 연결된 주문번호
    std::string sampleId;       // 생산할 시료 ID
    int         actualQuantity; // 실 생산량 (ceil(부족분 / (수율 × 0.9)))
    std::string enqueuedAt;     // 큐 등록 시각 ("YYYY-MM-DD HH:MM:SS")
    // Phase 3b에서 사용:
    std::string startedAt;      // 생산 시작 시각 (3a에서는 빈 문자열로 저장)
    double      totalDuration;  // 총 생산시간 시간 단위 (3a에서는 0.0으로 저장)

    nlohmann::json toJson() const;
    static ProductionJob fromJson(const nlohmann::json& j);
};
```

### JSON 필드 매핑

| 구조체 필드 | JSON 키 | 타입 |
|------------|---------|------|
| `orderNumber` | `"orderNumber"` | string |
| `sampleId` | `"sampleId"` | string |
| `actualQuantity` | `"actualQuantity"` | number (int) |
| `enqueuedAt` | `"enqueuedAt"` | string |
| `startedAt` | `"startedAt"` | string |
| `totalDuration` | `"totalDuration"` | number (double) |

---

## `ProductionQueue` 클래스

### 헤더 선언 (`Model/ProductionQueue.h` — 동일 파일에 선언)

```cpp
class ProductionQueue {
public:
    // filePath가 빈 문자열이면 파일 I/O 없이 in-memory로 동작 (테스트용).
    explicit ProductionQueue(const std::string& filePath);

    // 생산 작업 등록 (큐 뒤에 추가).
    // startedAt·totalDuration은 Phase 3b에서 enqueue 시 설정한다.
    void enqueue(const std::string& orderNumber,
                 const std::string& sampleId,
                 int actualQuantity);

    // 큐 앞 작업 조회 (제거하지 않음). 큐가 비어있으면 std::nullopt.
    std::optional<ProductionJob> front() const;

    // 큐 앞 작업 제거.
    void dequeue();

    // 대기 중인 전체 작업 목록 반환.
    std::vector<ProductionJob> getAll() const;

    bool empty() const;

private:
    std::string              filePath_;
    std::vector<ProductionJob> jobs_;

    void load();
    void save();
    std::string currentDateTimeString();
};
```

### 메서드 상세

#### `enqueue()`

```
job.orderNumber    = orderNumber
job.sampleId       = sampleId
job.actualQuantity = actualQuantity
job.enqueuedAt     = currentDateTimeString()
job.startedAt      = ""   // Phase 3b에서 설정
job.totalDuration  = 0.0  // Phase 3b에서 설정
jobs_.push_back(job)
save()
```

#### `front()`

```
jobs_.empty() → return std::nullopt
return jobs_.front()  (또는 jobs_[0])
```

#### `dequeue()`

```
if !jobs_.empty():
    jobs_.erase(jobs_.begin())
    save()
```

#### `getAll()`

```
return jobs_ 사본
```

### 영속성 설계

```
생성자
  └─ filePath_가 비어있지 않고 파일이 존재하면
       JSON 배열 파싱 → jobs_ 에 로드

save()
  └─ filePath_가 비어있지 않으면
       jobs_ 전체를 JSON 배열로 직렬화 → 파일 덮어쓰기
       (std::filesystem::create_directories로 data/ 자동 생성)
```

JSON 파일 형식 (`data/production.json`):

```json
[
  {
    "orderNumber": "ORD-20260612-0002",
    "sampleId": "S-001",
    "actualQuantity": 145,
    "enqueuedAt": "2026-06-12 16:00:00",
    "startedAt": "",
    "totalDuration": 0.0
  }
]
```

---

## 단위 테스트 구현 지침

테스트 파일: `Tests/ProductionQueueTest.cpp`

### in-memory 모드 사용

```cpp
ProductionQueue queue("");
```

### Phase 3a 테스트 구현 힌트

| 테스트 | 검증 포인트 |
|--------|------------|
| `작업을_등록하면_큐_뒤에_추가된다` | `enqueue()` 후 `getAll().back().orderNumber` 확인 |
| `먼저_등록된_작업이_먼저_처리된다_FIFO` | 2건 등록 후 `front()->orderNumber` == 첫 번째 확인 |
| `빈_큐에서_처리할_작업이_없으면_nullopt를_반환한다` | `front() == std::nullopt` |
| `작업_완료시_큐에서_제거된다` | `enqueue()` → `dequeue()` → `empty() == true` |
| `대기_중인_작업_목록을_반환한다` | 2건 등록 후 `getAll().size() == 2` |

### `OrderRepositoryTest.cpp` Phase 3a 추가 테스트 힌트

`OrderRepository.updateStatus()`를 활용한 상태 전환 테스트.

```cpp
// 픽스처: add() 후 updateStatus() 호출, findByNumber()로 상태 확인
TEST(OrderRepository, RESERVED에서_CONFIRMED로_상태를_변경한다) {
    OrderRepository repo("");
    auto o = repo.add("S-001", "고객", 100);
    EXPECT_TRUE(repo.updateStatus(o.orderNumber, OrderStatus::CONFIRMED));
    EXPECT_EQ(repo.findByNumber(o.orderNumber)->status, OrderStatus::CONFIRMED);
}
```

동일 패턴으로 PRODUCING, REJECTED, RELEASED 전환 테스트를 작성한다.
