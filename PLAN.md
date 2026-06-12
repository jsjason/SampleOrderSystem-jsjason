# PLAN — 구현 계획

> 기능 명세: [PRD.md](PRD.md) | 기술 설계: [CLAUDE.md](CLAUDE.md)

## 전략

- **TDD**: 각 Phase에서 테스트를 먼저 작성(RED) → 최소 구현(GREEN) → 정리(REFACTOR) 순으로 진행
- **점진적 릴리즈**: 각 Phase 완료 시 Release 빌드로 실제 기능을 수동 테스트 가능
- **레이어 순서**: Model → Controller → View 순으로 구현. View는 Model/Controller가 안정된 이후에 작성
- **Debug 빌드** → 단위 테스트 실행 / **Release 빌드** → 실제 앱 실행

---

## Phase 1 — 시료 관리

### 목표
시료를 등록하고 조회·검색할 수 있는 기반을 마련한다.
JSON 파일 영속성이 동작해야 하며, 프로그램 재시작 후에도 데이터가 유지되어야 한다.

### 구현 대상

| 파일 | 내용 |
|------|------|
| `json.h` | DataPersistence PoC에서 헤더 전용 JSON 파서 이식 |
| `Model/Sample.h/.cpp` | `Sample` 구조체, `toJson()`/`fromJson()`, `SampleRepository` (load/save/CRUD) |
| `Controller/SampleController.h/.cpp` | 시료 등록·목록 조회·이름 검색 메뉴 루프 |
| `View/SampleView.h/.cpp` | 시료 목록 테이블, 등록 입력 프롬프트, 검색 결과 출력 |
| `Controller/AppController.h/.cpp` | 메인 메뉴 루프 골격. [2]~[6] 메뉴는 "준비 중" 표시 |
| `View/MainView.h/.cpp` | 메인 메뉴 출력 (현황 요약은 Phase 4에서 완성) |
| `main.cpp` (Release 분기) | Repository 조립 및 `AppController::Run()` 호출 |

### 통과 단위 테스트 (10개)

`Tests/SampleRepositoryTest.cpp`

- `SampleRepository.시료를_등록하면_목록에서_조회된다`
- `SampleRepository.중복_ID로_등록하면_실패한다`
- `SampleRepository.등록된_모든_시료_목록을_반환한다`
- `SampleRepository.ID로_특정_시료를_조회한다`
- `SampleRepository.존재하지_않는_ID_조회시_nullopt를_반환한다`
- `SampleRepository.이름_키워드로_시료를_검색한다`
- `SampleRepository.대소문자_무관하게_이름을_검색한다`
- `SampleRepository.재고를_차감하면_stock이_감소한다`
- `SampleRepository.재고보다_많은_수량을_차감하면_실패한다`
- `SampleRepository.재고를_추가하면_stock이_증가한다`

### 수동 테스트 시나리오 (Release 빌드)

```
1. Release 빌드 실행
2. [1] 시료 관리 진입
3. [1] 시료 등록: ID=S-001, 이름=실리콘 웨이퍼-8인치, 평균생산시간=0.5, 수율=0.92
4. [1] 시료 등록: ID=S-002, 이름=GaN 에피택셜-4인치, 평균생산시간=0.3, 수율=0.78
5. [2] 시료 목록: S-001, S-002 표시 확인
6. [3] 시료 검색: "웨이퍼" 검색 → S-001만 표시 확인
7. 프로그램 종료 후 재실행 → 데이터 유지 확인
```

---

## Phase 2 — 시료 주문 접수

### 목표
고객 주문을 접수하여 `RESERVED` 상태로 저장한다.
주문번호가 자동 생성되고, 존재하지 않는 시료 ID로는 주문이 불가능해야 한다.

### 구현 대상

| 파일 | 내용 |
|------|------|
| `Model/Order.h/.cpp` | `OrderStatus` enum, `Order` 구조체, `toJson()`/`fromJson()`, `OrderRepository` (load/save/CRUD/filter) |
| `Controller/OrderController.h/.cpp` | 주문 접수 메뉴 (시료 ID 검증 → 주문 생성 → 저장) |
| `View/OrderView.h/.cpp` | 주문 입력 프롬프트, 접수 확인 화면, 주문 목록 테이블 |
| `Controller/AppController` (수정) | [2] 시료 주문 메뉴 연결 |

### 통과 단위 테스트 (6개)

`Tests/OrderRepositoryTest.cpp`

- `OrderRepository.주문을_접수하면_RESERVED_상태로_저장된다`
- `OrderRepository.주문번호는_ORD_날짜_시퀀스_형식으로_생성된다`
- `OrderRepository.전체_주문_목록을_반환한다`
- `OrderRepository.주문번호로_특정_주문을_조회한다`
- `OrderRepository.RESERVED_상태_주문만_필터링한다`
- `OrderRepository.CONFIRMED_상태_주문만_필터링한다`

### 수동 테스트 시나리오 (Release 빌드)

```
1. (Phase 1 데이터 유지 상태) Release 빌드 실행
2. [2] 시료 주문 진입
3. 시료 ID=S-001, 고객명=삼성전자 파운드리, 수량=100 입력
4. 주문번호 ORD-YYYYMMDD-NNNN 형식 자동 생성 확인
5. 상태 RESERVED 확인
6. 존재하지 않는 시료 ID로 주문 시도 → 오류 메시지 확인
```

---

## Phase 3a — 주문 승인/거절 + 생산 큐 등록

### 목표
`RESERVED` 주문을 승인하거나 거절한다. 승인 시 재고를 확인하여:
- 재고 충분 → `CONFIRMED` (즉시 출고 대기)
- 재고 부족 → 생산 큐에 자동 등록, `PRODUCING`

생산 완료 자동 처리(시간 기반)는 Phase 3b에서 구현한다.

### 구현 대상

| 파일 | 내용 |
|------|------|
| `Model/ProductionQueue.h/.cpp` | `ProductionJob` 구조체, `ProductionQueue` (FIFO enqueue/dequeue/peek/list). 시간 필드(`startedAt`, `totalDuration`)는 선언만, 자동 완료 로직은 Phase 3b에서 추가 |
| `Controller/OrderController` (수정) | 승인 로직: 재고 판단 → `SampleRepository.deductStock()` or 생산 큐 등록, 거절 로직 |
| `Controller/AppController` (수정) | [3] 주문 승인/거절 메뉴 연결 |

### 통과 단위 테스트 (19개)

`Tests/OrderRepositoryTest.cpp` (5개)
- `OrderRepository.RESERVED에서_CONFIRMED로_상태를_변경한다`
- `OrderRepository.RESERVED에서_PRODUCING으로_상태를_변경한다`
- `OrderRepository.RESERVED에서_REJECTED로_상태를_변경한다`
- `OrderRepository.CONFIRMED에서_RELEASED로_상태를_변경한다`
- `OrderRepository.PRODUCING에서_CONFIRMED로_상태를_변경한다`

`Tests/ProductionQueueTest.cpp` (5개)
- `ProductionQueue.작업을_등록하면_큐_뒤에_추가된다`
- `ProductionQueue.먼저_등록된_작업이_먼저_처리된다_FIFO`
- `ProductionQueue.빈_큐에서_처리할_작업이_없으면_nullopt를_반환한다`
- `ProductionQueue.작업_완료시_큐에서_제거된다`
- `ProductionQueue.대기_중인_작업_목록을_반환한다`

`Tests/OrderApprovalTest.cpp` (9개)
- `OrderApproval.재고가_충분하면_승인시_CONFIRMED로_전환된다`
- `OrderApproval.재고가_충분하면_승인시_재고가_주문량만큼_차감된다`
- `OrderApproval.재고가_충분하면_승인시_생산_큐에_등록되지_않는다`
- `OrderApproval.재고가_부족하면_승인시_PRODUCING으로_전환된다`
- `OrderApproval.재고가_부족하면_승인시_생산_큐에_자동_등록된다`
- `OrderApproval.재고가_부족하면_부족분만큼_생산_작업이_생성된다`
- `OrderApproval.재고가_0이면_주문량_전체가_생산_대상이다`
- `OrderApproval.거절하면_REJECTED로_전환된다`
- `OrderApproval.거절해도_재고는_변경되지_않는다`

### 수동 테스트 시나리오 (Release 빌드)

```
[재고 충분 경로]
1. S-001 시료 (재고 480ea) 에 대해 수량 100 주문 접수
2. [3] 주문 승인/거절 → 해당 주문 승인
3. 재고 확인: 480 → 380ea 차감 확인
4. 주문 상태 RESERVED → CONFIRMED 전환 확인

[재고 부족 경로]
1. S-001 시료 (재고 380ea) 에 대해 수량 500 주문 접수
2. [3] 주문 승인 → 부족분 120ea 생산 큐 등록 확인
3. 주문 상태 RESERVED → PRODUCING 전환 확인

[거절 경로]
1. 임의 주문 접수 후 [3]에서 거절 선택
2. 상태 REJECTED 전환 확인, 재고 변동 없음 확인
```

---

## Phase 3b — 시간 기반 자동 생산 완료 + 생산라인 조회

### 목표
실제 컴퓨터 시간을 기준으로 생산 작업을 자동 완료 처리한다.
생산 큐의 작업은 순차(FIFO)로 진행되며, 앞 작업 완료 후 다음 작업이 시작된다.
완료 시 재고 증가 및 주문 상태 전환(`PRODUCING → CONFIRMED`)이 자동으로 이루어진다.

**자동 완료 체크 호출 지점**: 재고 수량을 확인해야 하는 모든 진입 시점
- `AppController::run()` 루프 상단 (메인 메뉴 진입 시)
- `OrderController::handleApproveOrder()` 진입 시
- `ProductionController::run()` 루프 상단 (생산라인 조회 진입 시)

**순차 처리 시간 계산**:
- 큐 앞 작업의 `startedAt + totalDuration <= now` → 완료 처리
- 다음 작업의 `startedAt` = 이전 작업의 `startedAt + totalDuration` (now 기준 아님)
- 폴링이 늦어도 시간 계산 정확성 보장; 복수 작업이 동시에 완료될 수 있음

**실 생산량 계산**: `ceil(부족분 / (수율 × 0.9))`
**총 생산시간**: `평균생산시간 × 실생산량` (단위: 시간)

### 구현 대상

| 파일 | 내용 |
|------|------|
| `Model/ProductionQueue.h/.cpp` (수정) | `ProductionJob`에 `startedAt`, `totalDuration` 추가. `processCompleted(SampleRepository&, OrderRepository&)` 메서드 구현 |
| `Controller/ProductionController.h/.cpp` | 생산라인 조회 (현재 작업 + 대기 큐 목록, 예상 완료 시각) |
| `View/ProductionView.h/.cpp` | 생산 현황 테이블, 대기 큐 목록 출력 |
| `Controller/AppController` (수정) | 루프 상단에 `processCompleted()` 호출, [5] 생산라인 조회 메뉴 연결 |
| `Controller/OrderController` (수정) | `handleApproveOrder()` 진입 시 `processCompleted()` 호출 |

### 통과 단위 테스트 (10개)

`Tests/ProductionQueueTest.cpp` (4개)
- `ProductionCalculation.부족분과_수율로_실_생산량을_계산한다`
- `ProductionCalculation.수율_0_92_부족분_170일때_실생산량은_206이다`
- `ProductionCalculation.총_생산시간은_평균생산시간_곱하기_실생산량이다`
- `ProductionCalculation.부족분이_0이면_생산_작업을_등록하지_않는다`

`Tests/ProductionQueueTest.cpp` — 자동 완료 (3개)
- `ProductionQueue.경과_시간이_충분하면_완료_작업이_자동_처리된다`
- `ProductionQueue.순차_처리시_다음_작업의_startedAt은_이전_작업_완료시각이다`
- `ProductionQueue.경과_시간이_부족하면_작업이_처리되지_않는다`

`Tests/OrderApprovalTest.cpp` (3개)
- `OrderApproval.생산_완료시_PRODUCING에서_CONFIRMED로_전환된다`
- `OrderApproval.생산_완료시_시료_재고가_실생산량만큼_증가한다`
- `OrderApproval.생산_완료시_재고에서_주문량만큼_차감된다`

### 수동 테스트 시나리오 (Release 빌드)

```
[생산 자동 완료 경로]
1. (Phase 3a 재고 부족 경로 이후) PRODUCING 상태 주문 존재
2. [5] 생산라인 조회 → 현재 작업, 대기 큐, 예상 완료 시각 확인
3. 충분한 시간 경과 후 메인 메뉴 재진입 또는 [5] 재진입
4. 자동 완료 처리 → 재고 증가, 주문 상태 PRODUCING → CONFIRMED 전환 확인

[복수 작업 순차 완료]
1. PRODUCING 작업이 있는 상태에서 추가 주문 승인 → 큐에 두 번째 작업 등록 확인
2. 충분한 시간 경과 후 첫 번째 작업 완료 → 두 번째 작업 자동 시작 확인
```

---

## Phase 4 — 모니터링 + 출고 처리 + 메인 현황 완성

### 목표
전체 시스템을 완성한다. 모니터링으로 주문 현황과 재고 상태를 한눈에 확인하고,
`CONFIRMED` 주문을 출고(`RELEASED`)로 처리할 수 있다.
메인 메뉴의 현황 요약(시료 수, 총 재고, 주문 수, 생산 대기)도 완성한다.

### 구현 대상

| 파일 | 내용 |
|------|------|
| `Controller/MonitorController.h/.cpp` | 상태별 주문 건수 집계, 시료별 재고 상태(여유/부족/고갈) 판단 |
| `View/MonitorView.h/.cpp` | 주문 현황 + 재고 현황 대시보드 출력 |
| `Controller/ReleaseController.h/.cpp` | `CONFIRMED` 주문 목록 표시 → 선택 → `RELEASED` 전환 |
| `View/ReleaseView.h/.cpp` | 출고 가능 주문 목록, 출고 처리 결과 출력 |
| `View/MainView` (수정) | 현황 요약 출력 (등록 시료 수, 총 재고, 전체 주문 수, 생산 대기) |
| `Controller/AppController` (수정) | [4] 모니터링, [6] 출고 처리 메뉴 연결, 현황 데이터 전달 |

### 통과 단위 테스트 (1개)

`Tests/OrderApprovalTest.cpp`

- `OrderApproval.CONFIRMED_주문을_출고하면_RELEASED로_전환된다`

### 수동 테스트 시나리오 (Release 빌드)

```
[모니터링]
1. 여러 주문을 다양한 상태로 만든 뒤 [4] 모니터링 진입
2. 상태별 주문 건수 (RESERVED/CONFIRMED/PRODUCING/RELEASED) 확인
3. 시료별 재고 상태 (여유/부족/고갈) 표기 확인

[출고 처리]
1. CONFIRMED 상태 주문이 있는 상태에서 [6] 출고 처리 진입
2. CONFIRMED 목록 확인 후 특정 주문 선택
3. 출고 처리 후 상태 CONFIRMED → RELEASED 전환 확인

[메인 현황 요약]
1. 메인 메뉴 상단에 현재 시료 수, 총 재고, 전체 주문 수, 생산 대기 수 표시 확인
```

---

## 단위 테스트 완료 기준 요약

| Phase | 새로 통과하는 테스트 | 누적 통과 |
|-------|-------------------|---------|
| Phase 1  | SampleRepository 10개 | **10 / 46** |
| Phase 2  | OrderRepository 6개 | **16 / 46** |
| Phase 3a | OrderRepository 5개 + ProductionQueue 5개 + OrderApproval 9개 | **35 / 46** |
| Phase 3b | ProductionCalculation 4개 + ProductionQueue(자동완료) 3개 + OrderApproval(생산완료) 3개 | **45 / 46** |
| Phase 4  | OrderApproval 1개 | **46 / 46** |

Phase 4 완료 시 전체 46개 테스트 PASS, 0개 SKIPPED.
