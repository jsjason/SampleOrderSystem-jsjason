# Phase 1 설계 — Model 레이어

## 개요

`Sample` 구조체와 `SampleRepository` 클래스를 설계한다.
Model 레이어는 View/Controller를 전혀 참조하지 않으며, JSON 파일 영속성을 자체적으로 처리한다.

---

## 파일 구성

```
SampleOrderSystem-jsjason/
├── json.h                  ← DataPersistence PoC에서 이식 (변경 없음)
└── Model/
    ├── Sample.h
    └── Sample.cpp
```

---

## `Sample` 구조체

### 헤더 선언 (`Model/Sample.h`)

```cpp
#pragma once
#include <string>
#include "../json.h"

struct Sample {
    std::string id;
    std::string name;
    double      avgProductionTime;  // 평균 생산시간 (min/ea)
    double      yield;              // 수율 0.0~1.0
    int         stock;              // 현재 재고 (ea)

    nlohmann::json toJson() const;
    static Sample  fromJson(const nlohmann::json& j);
};
```

> `json.h`가 `nlohmann::json`을 노출한다고 가정한다. PoC를 이식할 때 실제 타입명을 확인하여 맞춘다.

### JSON 필드 매핑

| 구조체 필드 | JSON 키 | 타입 |
|------------|---------|------|
| `id` | `"id"` | string |
| `name` | `"name"` | string |
| `avgProductionTime` | `"avgProductionTime"` | number |
| `yield` | `"yield"` | number |
| `stock` | `"stock"` | number (int) |

### 구현 스케치 (`Model/Sample.cpp`)

```cpp
nlohmann::json Sample::toJson() const {
    return {
        {"id",                 id},
        {"name",               name},
        {"avgProductionTime",  avgProductionTime},
        {"yield",              yield},
        {"stock",              stock}
    };
}

Sample Sample::fromJson(const nlohmann::json& j) {
    return {
        j.at("id").get<std::string>(),
        j.at("name").get<std::string>(),
        j.at("avgProductionTime").get<double>(),
        j.at("yield").get<double>(),
        j.at("stock").get<int>()
    };
}
```

---

## `SampleRepository` 클래스

### 헤더 선언 (`Model/Sample.h` — 동일 파일에 선언)

```cpp
#include <vector>
#include <optional>

class SampleRepository {
public:
    // filePath가 빈 문자열("")이면 파일 I/O를 수행하지 않는다 (테스트용 in-memory 모드).
    explicit SampleRepository(const std::string& filePath);

    // 등록. 동일 id가 이미 존재하면 false 반환.
    bool add(const Sample& sample);

    // 전체 목록 반환.
    std::vector<Sample> getAll() const;

    // ID 조회. 없으면 std::nullopt 반환.
    std::optional<Sample> findById(const std::string& id) const;

    // 이름 대소문자 무관 부분 일치 검색.
    std::vector<Sample> searchByName(const std::string& keyword) const;

    // 재고 차감. 재고 부족이면 false 반환(변경 없음).
    bool deductStock(const std::string& id, int quantity);

    // 재고 추가.
    void addStock(const std::string& id, int quantity);

private:
    std::string         filePath_;
    std::vector<Sample> samples_;

    void load();   // 생성자에서 호출. filePath_가 비었으면 no-op.
    void save();   // 변경 메서드 호출 후 즉시 호출. filePath_가 비었으면 no-op.
};
```

### 메서드 상세

#### `add()`

```
1. samples_ 를 순회하여 id 중복 검사 → 있으면 false 반환
2. samples_.push_back(sample)
3. save()
4. return true
```

#### `getAll()`

```
samples_ 사본 반환 (const, 내부 벡터 직접 노출 금지)
```

#### `findById()`

```
samples_ 를 순회하여 id 일치 항목 반환
없으면 std::nullopt
```

#### `searchByName()`

```
keyword를 소문자로 변환
samples_ 를 순회하면서 name 소문자 변환 후 keyword 포함 여부 확인
일치하는 항목들을 모아 반환
```

소문자 변환 헬퍼 (익명 네임스페이스 또는 static 함수):
```cpp
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
```

#### `deductStock()`

```
findById(id) → 없으면 false
sample.stock < quantity → false (변경 없음)
sample.stock -= quantity
save()
return true
```

> `samples_` 벡터에서 해당 원소를 직접 수정해야 한다 (`std::find_if` + 이터레이터 참조 또는 인덱스 사용).

#### `addStock()`

```
findById(id) → 없으면 return (무시)
sample.stock += quantity
save()
```

### 영속성 설계

```
생성자
  └─ filePath_가 비어있지 않고 파일이 존재하면
       JSON 배열 파싱 → samples_ 에 로드

save()
  └─ filePath_가 비어있지 않으면
       samples_ 전체를 JSON 배열로 직렬화 → 파일 덮어쓰기
```

JSON 파일 형식:
```json
[
  {
    "id": "S-001",
    "name": "실리콘 웨이퍼-8인치",
    "avgProductionTime": 0.5,
    "yield": 0.92,
    "stock": 480
  }
]
```

---

## 단위 테스트 구현 지침

테스트 파일: `Tests/SampleRepositoryTest.cpp`

### in-memory 모드 사용

```cpp
SampleRepository repo("");  // filePath = "" → 파일 I/O 없음
```

각 `TEST()` 내부에서 `repo`를 로컬로 생성하면 테스트 간 상태 공유가 없다.

### 픽스처 패턴 (반복 사용하는 시료)

```cpp
static Sample makeSample(const std::string& id = "S-001",
                         const std::string& name = "테스트 시료",
                         int stock = 100) {
    return {id, name, 0.5, 0.92, stock};
}
```

### 각 테스트 구현 힌트

| 테스트 | 검증 포인트 |
|--------|------------|
| `시료를_등록하면_목록에서_조회된다` | `add()` true, `findById()` 반환값 확인 |
| `중복_ID로_등록하면_실패한다` | 두 번째 `add()` → false |
| `등록된_모든_시료_목록을_반환한다` | `getAll().size() == 2` |
| `ID로_특정_시료를_조회한다` | `findById("S-001").has_value()` + 필드 값 확인 |
| `존재하지_않는_ID_조회시_nullopt를_반환한다` | `findById("X") == std::nullopt` |
| `이름_키워드로_시료를_검색한다` | `searchByName("웨이퍼").size() == 1` |
| `대소문자_무관하게_이름을_검색한다` | 영문 포함 이름에 대해 대소문자 변형으로 검색 |
| `재고를_차감하면_stock이_감소한다` | `deductStock()` true, `findById().stock` 감소 확인 |
| `재고보다_많은_수량을_차감하면_실패한다` | `deductStock()` false, `stock` 불변 확인 |
| `재고를_추가하면_stock이_증가한다` | `addStock()` 후 `findById().stock` 증가 확인 |
