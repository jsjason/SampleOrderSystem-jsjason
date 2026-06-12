# Phase 3b 설계 — Model 레이어

## 개요

`ProductionQueue`에 시간 기반 자동 완료 로직을 추가한다.
Phase 3a에서 선언만 해 둔 `startedAt`, `totalDuration` 필드를 `enqueue()` 시점에 계산하여 채우고,
`processCompleted()`를 신규 구현하여 경과 시간을 기준으로 완료 처리한다.

---

## 파일 구성

```
SampleOrderSystem-jsjason/
└── Model/
    ├── ProductionQueue.h   ← enqueue() 시그니처 변경, processCompleted() 추가
    └── ProductionQueue.cpp ← 구현
```

---

## `enqueue()` 시그니처 변경

`totalDuration` 계산에 `avgProductionTime`이 필요하므로 파라미터로 추가한다.

```cpp
// Phase 3a
void enqueue(const std::string& orderNumber,
             const std::string& sampleId,
             int                actualQuantity);

// Phase 3b (변경 후)
void enqueue(const std::string& orderNumber,
             const std::string& sampleId,
             int                actualQuantity,
             double             avgProductionTime);  // 단위: 분/ea
```

호출 측(`applyApproval()`)은 이미 `sample.avgProductionTime`에 접근 가능하므로 그대로 전달한다.

> **기존 FIFO 단위 테스트 수정 필요**: Phase 3a의 ProductionQueue FIFO 테스트가
> 3-파라미터 `enqueue()`를 호출하고 있으므로 `avgProductionTime = 0.0`을 추가한다.
> 타이밍과 무관한 테스트이므로 동작에는 영향 없다.

---

## `enqueue()` 내부 계산

### `totalDuration`

```
totalDuration = avgProductionTime(분/ea) × actualQuantity × 60.0   // 단위: 초
```

초 단위로 저장하면 `std::difftime(now, startedAt)` 반환값과 직접 비교 가능하다.

### `startedAt`

큐가 비어 있으면 즉시 시작, 이미 작업이 있으면 마지막 작업 완료 시각을 이어받는다.
폴링이 늦어도 시간 계산 정확성이 보장되는 핵심 불변식이다.

```
if jobs_.empty():
    job.startedAt = currentDateTimeString()   // 지금 바로 시작
else:
    last = jobs_.back()
    newStart = parseDateTime(last.startedAt) + (time_t)last.totalDuration
    job.startedAt = formatDateTime(newStart)  // 이전 작업 완료 직후 시작
```

---

## `processCompleted()` — 신규 메서드

### 선언

```cpp
// <ctime> 필요. now 기본값으로 단위 테스트 시 타임스탬프 주입 가능.
void processCompleted(SampleRepository& sampleRepo,
                      OrderRepository&  orderRepo,
                      std::time_t       now = std::time(nullptr));
```

### 처리 흐름

생산 완료를 일괄 처리하지 않고, **호출될 때마다 경과 시간에 비례하여 재고를 점진적으로 반영**한다.
`creditedQuantity`가 이미 재고에 반영된 수량을 추적하므로 중복 반영이 없다.

```
while jobs_ 비어있지 않음:
    job = jobs_.front()
    startedAt_t = parseDateTime(job.startedAt)
    elapsed = difftime(now, startedAt_t)

    if elapsed < 0:
        break

    // 점진적 재고 반영
    if job.actualQuantity > 0 and job.totalDuration > 0:
        timePerItem  = job.totalDuration / job.actualQuantity    // 개당 생산 소요시간(초)
        producedSoFar = min(floor(elapsed / timePerItem), job.actualQuantity)
    else:
        producedSoFar = 0

    newlyProduced = producedSoFar - job.creditedQuantity
    if newlyProduced > 0:
        sampleRepo.addStock(job.sampleId, newlyProduced)
        job.creditedQuantity += newlyProduced
        save()

    if producedSoFar < job.actualQuantity:
        break   // 순차 처리: 맨 앞이 미완료이면 나머지도 미완료

    // 작업 완료 처리
    order = orderRepo.findByNumber(job.orderNumber)
    if order.has_value():
        sampleRepo.deductStock(job.sampleId, order.quantity)    // 주문분 재고 차감
        orderRepo.updateStatus(job.orderNumber, CONFIRMED)

    jobs_.erase(jobs_.begin())
    save()
```

복수 작업이 동시에 완료 조건을 충족하면 루프가 계속 돌아 모두 처리된다.

---

## 헬퍼 함수 — `parseDateTime` / `formatDateTime`

`ProductionQueue.cpp` 내 정적 헬퍼로 구현한다.

```cpp
// #include <sstream>, <iomanip> 필요
static std::time_t parseDateTime(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

// localtime_s: MSVC 전용 (Windows x64 타깃이므로 사용 가능)
static std::string formatDateTime(std::time_t t) {
    std::tm tm = {};
    localtime_s(&tm, &t);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
```

`currentDateTimeString()`은 기존 그대로 유지한다.
`formatDateTime()`은 `startedAt` 계산 시 `enqueue()` 내부에서만 사용한다.

---

## 업데이트된 `ProductionQueue` 선언 요약

```cpp
struct ProductionJob {
    std::string orderNumber;
    std::string sampleId;
    int         actualQuantity;
    std::string enqueuedAt;
    std::string startedAt;
    double      totalDuration;
    int         creditedQuantity = 0;  // 재고에 이미 반영된 수량 (Phase 3b 추가)

    nlohmann::json toJson() const;
    static ProductionJob fromJson(const nlohmann::json& j);
};

class ProductionQueue {
public:
    explicit ProductionQueue(const std::string& filePath);

    void enqueue(const std::string& orderNumber,
                 const std::string& sampleId,
                 int                actualQuantity,
                 double             avgProductionTime);  // Phase 3b: 파라미터 추가

    std::optional<ProductionJob> front() const;
    void dequeue();
    std::vector<ProductionJob> getAll() const;
    bool empty() const;

    // Phase 3b 신규
    void processCompleted(SampleRepository& sampleRepo,
                          OrderRepository&  orderRepo,
                          std::time_t       now = std::time(nullptr));

private:
    std::string                filePath_;
    std::vector<ProductionJob> jobs_;
    void        load();
    void        save();
    std::string currentDateTimeString();
};
```

---

## 단위 테스트 구현 지침

테스트 파일: `Tests/ProductionQueueTest.cpp`

### 타임스탬프 주입 패턴

```cpp
// enqueue 후 job의 startedAt을 파싱해서 future 타임스탬프 계산
ProductionQueue q("");
q.enqueue("ORD-001", "S-001", 10, 0.01);   // totalDuration = 6초

auto job = q.front().value();
std::time_t start = parseDateTime(job.startedAt);  // 테스트 내 동일 헬퍼 필요
// 또는 job.startedAt을 sscanf로 직접 파싱

std::time_t future = start + (std::time_t)job.totalDuration + 1;
q.processCompleted(sampleRepo, orderRepo, future);
```

> `parseDateTime`은 `ProductionQueue.cpp` 내 정적 함수이므로 테스트에서 직접 호출할 수 없다.
> 테스트 픽스처에서 같은 로직을 인라인으로 구현하거나,
> `job.startedAt`을 `sscanf`로 파싱한다.

### Phase 3b 테스트 구현 힌트

| 테스트 | 검증 포인트 |
|--------|------------|
| `부족분과_수율로_실_생산량을_계산한다` | `calculateActualQuantity(shortage, yield)` 반환값 확인 |
| `수율_0_92_부족분_170일때_실생산량은_206이다` | `ceil(170 / (0.92 × 0.9))` = 206 확인 |
| `총_생산시간은_평균생산시간_분_곱하기_실생산량_곱하기_60초이다` | `front()->totalDuration == avgProductionTime * qty * 60.0` |
| `부족분이_0이면_실생산량은_0이다` | `calculateActualQuantity(0, yield) == 0` (shortage ≤ 0 조기 반환) |
| `경과_시간이_충분하면_완료_작업이_자동_처리된다` | `future = startedAt + totalDuration + 1` 주입 → `empty() == true` |
| `순차_처리시_다음_작업의_startedAt은_이전_작업_완료시각이다` | 2건 enqueue → 두 번째 `startedAt == 첫 번째 startedAt + 첫 번째 totalDuration` |
| `경과_시간이_부족하면_작업이_처리되지_않는다` | `now = startedAt + totalDuration - 1` → `empty() == false` |

### `OrderApprovalTest.cpp` 추가 테스트 힌트

```cpp
// 픽스처에서 applyApproval() 후 processCompleted() 호출
TEST_F(OrderApproval, 생산_완료시_PRODUCING에서_CONFIRMED로_전환된다) {
    addSample("S-001", 0, 0.92);           // 재고 0 → 전량 생산
    auto num = placeOrder("S-001", 10);
    applyApproval(num, sampleRepo_, orderRepo_, prodQueue_);

    auto job    = prodQueue_.front().value();
    auto start  = /* job.startedAt 파싱 */;
    auto future = start + (std::time_t)job.totalDuration + 1;
    prodQueue_.processCompleted(sampleRepo_, orderRepo_, future);

    EXPECT_EQ(orderRepo_.findByNumber(num)->status, OrderStatus::CONFIRMED);
}
```
