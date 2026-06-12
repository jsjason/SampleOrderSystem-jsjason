# Phase Xtra 설계 — 더미 데이터 생성 ([7])

## 개요

메인 메뉴에 `[7] 더미 데이터 생성` 항목을 추가한다.
실행하면 `data/samples.json`, `data/orders.json`, `data/production.json`을 고정된 더미 데이터로 덮어쓴다.
개발·시연 시 초기 데이터를 빠르게 세팅하는 용도.

---

## 파일 구성 변경

```
SampleOrderSystem-jsjason/
├── View/
│   └── DummyView.h/.cpp             ← 신규
├── Controller/
│   └── DummyController.h/.cpp       ← 신규
├── Model/
│   ├── Sample.h/.cpp                ← 수정 (replaceAll 추가)
│   ├── Order.h/.cpp                 ← 수정 (replaceAll 추가)
│   └── ProductionQueue.h/.cpp       ← 수정 (clear 추가)
├── View/MainView.h/.cpp             ← 수정 ([7] 메뉴 항목 추가)
├── Controller/AppController.h/.cpp  ← 수정 (DummyController 멤버 추가, case 7 연결)
└── main.cpp                         ← 수정 (DummyController 조립)
```

---

## 더미 데이터 명세

### samples.json — 5종 시료

| ID    | 시료명              | avgProductionTime | yield | stock |
|-------|---------------------|-------------------|-------|-------|
| S-001 | 실리콘 웨이퍼-8인치  | 0.5               | 0.92  | 480   |
| S-002 | GaN 에피택셜-4인치  | 1.2               | 0.85  | 0     |
| S-003 | GaAs 웨이퍼-6인치   | 0.8               | 0.88  | 200   |
| S-004 | SiC 기판-4인치      | 2.0               | 0.75  | 320   |
| S-005 | InP 기판-2인치      | 3.5               | 0.70  | 60    |

### orders.json — 11건 (PRODUCING 제외, 4종 고르게)

| 주문번호              | 시료  | 고객명              | 수량 | 상태      |
|-----------------------|-------|---------------------|------|-----------|
| ORD-20260501-0001     | S-001 | 삼성전자 파운드리    | 100  | RESERVED  |
| ORD-20260502-0001     | S-003 | SK하이닉스 연구소    |  80  | RESERVED  |
| ORD-20260503-0001     | S-005 | 한화시스템 연구소    |  40  | RESERVED  |
| ORD-20260510-0001     | S-001 | LG이노텍 개발팀      | 150  | CONFIRMED |
| ORD-20260511-0001     | S-003 | 한국반도체연구원     | 120  | CONFIRMED |
| ORD-20260512-0001     | S-004 | 인텔코리아 팹리스    | 200  | CONFIRMED |
| ORD-20260520-0001     | S-002 | 미래나노텍           | 100  | REJECTED  |
| ORD-20260521-0001     | S-003 | 서울대학교 물리학과  |  50  | REJECTED  |
| ORD-20260601-0001     | S-001 | 삼성전자 파운드리    | 200  | RELEASED  |
| ORD-20260602-0001     | S-002 | SK하이닉스 연구소    |  50  | RELEASED  |
| ORD-20260603-0001     | S-005 | 포스코홀딩스 기술원  |  30  | RELEASED  |

### production.json

빈 배열 `[]` — 더미 데이터 생성 시 생산 큐를 초기화한다.

---

## Model 변경 — `replaceAll` / `clear`

더미 데이터를 Repository에 반영하려면 기존 데이터를 교체하는 메서드가 필요하다.

```cpp
// Sample.h
void SampleRepository::replaceAll(const std::vector<Sample>& samples);

// Order.h
void OrderRepository::replaceAll(const std::vector<Order>& orders);

// ProductionQueue.h
void ProductionQueue::clear();
```

구현은 기존 `save()` 패턴을 그대로 따른다.

```cpp
void SampleRepository::replaceAll(const std::vector<Sample>& samples) {
    samples_ = samples;
    save();
}

void OrderRepository::replaceAll(const std::vector<Order>& orders) {
    orders_ = orders;
    save();
}

void ProductionQueue::clear() {
    jobs_.clear();
    save();
}
```

---

## `DummyView` — 신규

### 헤더 (`View/DummyView.h`)

```cpp
class DummyView {
public:
    bool promptConfirm() const;           // 덮어쓰기 확인 (y/n)
    void showSuccess(int sampleCount, int orderCount) const;
};
```

### 화면 레이아웃

**확인 프롬프트**
```
===========================================
  더미 데이터 생성
===========================================
  기존 데이터(시료, 주문, 생산 큐)가 모두 초기화됩니다.
  계속하시겠습니까? (y/n) >
```

**완료 메시지**
```
  더미 데이터가 생성되었습니다.
  시료 5개 / 주문 11건이 등록되었습니다.
  [ Enter 키를 누르면 계속합니다 ]
```

---

## `DummyController` — 신규

### 헤더 (`Controller/DummyController.h`)

```cpp
class DummyController {
public:
    DummyController(SampleRepository&, OrderRepository&,
                    ProductionQueue&,  DummyView&);
    void run();

private:
    void generate();
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    DummyView&        view_;
};
```

### 흐름 (`run()`)

```
run():
    if not view_.promptConfirm(): return

    generate()
    view_.showSuccess(5, 11)
```

### `generate()` 구현 요점

```
generate():
    // 시료 5종 구성
    samples = [ {S-001, ...}, {S-002, ...}, ... ]
    sampleRepo_.replaceAll(samples)

    // 주문 11건 구성 (orderNumber, createdAt 하드코딩)
    orders = [ {ORD-20260501-0001, ...RESERVED}, ... ]
    orderRepo_.replaceAll(orders)

    // 생산 큐 초기화
    prodQueue_.clear()
```

---

## `MainView` 수정

```cpp
// 추가
std::cout << "  " << LABEL << "[7]" << RESET << " 더미 데이터 생성\n";
```

---

## `AppController` 수정

```cpp
// 생성자 파라미터 추가
DummyController& dummyCtrl_;

// case 추가
case 7: dummyCtrl_.run(); break;
```

---

## Phase Xtra 완료 기준 체크리스트

- [ ] [7] 선택 시 확인 프롬프트 표시
- [ ] n 입력 시 메인 메뉴로 복귀 (데이터 변경 없음)
- [ ] y 입력 시 samples.json 5건, orders.json 11건 생성
- [ ] 생산 큐(production.json) 빈 배열로 초기화
- [ ] 생성 후 메인 현황 수치(시료 5개, 주문 11건 등) 즉시 갱신 확인
- [ ] 기존 데이터가 있어도 완전 덮어쓰기 동작 확인
