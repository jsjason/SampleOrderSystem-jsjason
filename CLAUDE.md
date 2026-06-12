# SampleOrderSystem-jsjason

가상의 반도체 회사 "S-Semi"의 시료(Sample) 생산주문관리 시스템.
콘솔 기반 C++ 애플리케이션으로, MVC 패턴 + JSON 파일 영속성을 사용한다.

## 개발 환경

- **IDE**: Visual Studio 2022 (v18)
- **언어**: C++20
- **플랫폼**: Windows x64 (Debug / Release)
- **툴체인**: MSVC v145
- **빌드**: MSBuild

### 빌드 명령

```powershell
# vswhere로 MSBuild 경로 자동 탐색
$msbuild = &"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe `
    | Select-Object -First 1
& $msbuild SampleOrderSystem-jsjason\SampleOrderSystem-jsjason.vcxproj `
    /p:Configuration=Debug /p:Platform=x64 /t:Rebuild /nologo /v:minimal
```

빌드 결과물: `SampleOrderSystem-jsjason\x64\Debug\SampleOrderSystem-jsjason.exe`

## 프로젝트 구조

```
SampleOrderSystem-jsjason/          ← git 저장소 루트
├── CLAUDE.md
├── SampleOrderSystem-jsjason.slnx  ← Visual Studio 솔루션
└── SampleOrderSystem-jsjason/      ← C++ 프로젝트 디렉터리
    ├── main.cpp                    ← 진입점. 의존성 조립, 콘솔 인코딩 초기화
    ├── json.h                      ← 헤더 전용 JSON 파서/직렬화기 (외부 의존성 없음)
    ├── Model/
    │   ├── Sample.h/.cpp           ← 시료 엔티티 + SampleRepository
    │   ├── Order.h/.cpp            ← 주문 엔티티 (OrderStatus enum) + OrderRepository
    │   └── ProductionQueue.h/.cpp  ← 생산 큐 (FIFO) + 생산량 계산 로직
    ├── View/
    │   ├── MainView.h/.cpp         ← 메인 메뉴, 시스템 현황 요약
    │   ├── SampleView.h/.cpp       ← 시료 관리 화면
    │   ├── OrderView.h/.cpp        ← 주문 접수 / 승인·거절 화면
    │   ├── MonitorView.h/.cpp      ← 모니터링 대시보드
    │   ├── ProductionView.h/.cpp   ← 생산라인 현황 화면
    │   └── ReleaseView.h/.cpp      ← 출고 처리 화면
    └── Controller/
        ├── AppController.h/.cpp    ← 메인 루프, 메뉴 라우팅
        ├── SampleController.h/.cpp ← 시료 등록·조회·검색
        ├── OrderController.h/.cpp  ← 주문 접수, 승인/거절, 재고 판단
        ├── MonitorController.h/.cpp← 모니터링 조회
        ├── ProductionController.h/.cpp ← 생산라인 조회, 생산 완료 처리
        └── ReleaseController.h/.cpp    ← 출고 처리
```

## 도메인 모델

### Sample (시료)

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `string` | 시료 ID (예: `S-001`) |
| `name` | `string` | 시료명 (예: `실리콘 웨이퍼-8인치`) |
| `avgProductionTime` | `double` | 평균 생산시간 (min/ea) |
| `yield` | `double` | 수율 (0.0 ~ 1.0, 예: 0.92) |
| `stock` | `int` | 현재 재고 수량 (ea) |

### Order (주문)

| 필드 | 타입 | 설명 |
|------|------|------|
| `orderNumber` | `string` | 주문번호 (예: `ORD-20260416-0043`) |
| `sampleId` | `string` | 시료 ID |
| `customerName` | `string` | 고객명 |
| `quantity` | `int` | 주문 수량 |
| `status` | `OrderStatus` | 주문 상태 |
| `createdAt` | `string` | 접수 일시 |

### OrderStatus (주문 상태)

```
RESERVED   → 주문 접수 (초기 상태)
    ├── [거절] → REJECTED  (모니터링 제외)
    └── [승인]
         ├── 재고 충분 → CONFIRMED  (출고 대기)
         └── 재고 부족 → PRODUCING  (생산 중)
                              ↓ 생산 완료
                          CONFIRMED
                              ↓ 출고 처리
                           RELEASED
```

### ProductionJob (생산 작업)

생산 큐에 등록되는 단위. 주문 승인 시 재고 부족이면 자동 생성.

| 필드 | 설명 |
|------|------|
| `orderId` | 연결된 주문번호 |
| `sampleId` | 생산할 시료 ID |
| `shortage` | 부족분 (주문량 - 재고) |
| `actualProduction` | 실 생산량 = `ceil(부족분 / (수율 × 0.9))` |
| `totalTime` | 총 생산 시간 = `평균 생산시간 × 실 생산량` (min) |

## 핵심 비즈니스 로직

### 주문 승인 흐름

```
승인 요청 (RESERVED 주문)
    ↓
재고 확인: stock >= quantity ?
    ├── YES → stock -= quantity, 상태 = CONFIRMED
    └── NO  → 부족분 = quantity - stock
              실 생산량 = ceil(부족분 / (수율 × 0.9))
              생산 큐에 등록 (FIFO)
              상태 = PRODUCING
```

### 생산 완료 처리

- 생산 완료 시 해당 시료의 `stock += 실 생산량`
- 연결된 주문에서 `quantity`만큼 재고 차감
- 주문 상태를 `PRODUCING → CONFIRMED`로 변경

### 재고 상태 표기 (모니터링)

- **여유**: `stock > 0` 이고 주문대비 재고 충분
- **부족**: `stock > 0` 이지만 주문대비 재고 부족
- **고갈**: `stock == 0`

## 데이터 영속성

JSON 파일 방식. 애플리케이션 종료 후 재시작해도 데이터 유지.

```
data/
├── samples.json   ← 시료 목록
├── orders.json    ← 전체 주문 목록
└── production.json ← 생산 큐 상태
```

각 Repository는 생성자에서 파일을 로드하고, 변경 시마다 즉시 파일을 덮어쓴다.
`json.h`는 DataPersistence-jsjason PoC의 헤더 전용 파서를 그대로 재사용한다.

### JSON 스키마

**samples.json**
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

**orders.json**
```json
[
  {
    "orderNumber": "ORD-20260416-0043",
    "sampleId": "S-003",
    "customerName": "삼성전자 파운드리",
    "quantity": 200,
    "status": "PRODUCING",
    "createdAt": "2026-04-16 09:32:15"
  }
]
```

**production.json**
```json
[
  {
    "orderId": "ORD-20260416-0043",
    "sampleId": "S-003",
    "shortage": 170,
    "actualProduction": 206,
    "totalTime": 165.0,
    "enqueuedAt": "2026-04-16 09:32:20"
  }
]
```

## MVC 레이어 역할

### Model

데이터 구조 + 저장소 로직만 담당. View/Controller를 전혀 참조하지 않는다.

- `Sample`, `Order`, `ProductionJob` 구조체: `toJson()` / `fromJson()` 포함
- `SampleRepository`, `OrderRepository`, `ProductionQueue`: 생성자 로드, 변경 시 자동 저장

### View

콘솔 렌더링과 사용자 입력 수집만 담당. 비즈니스 로직 없음.

- `Show*()` 메서드: 화면 출력
- `Prompt*()` 메서드: 사용자 입력 수집
- 공통 유틸: 구분선, 상태 배지, 테이블 출력

### Controller

Model과 View를 연결. 입력 → 비즈니스 로직 → 결과 표시.

- `AppController::Run()` — 최상위 메뉴 루프
- 각 도메인 Controller가 하위 메뉴 루프 처리

### main.cpp

세 레이어를 조립하고 실행. 의존성은 생성자 참조 주입.

```cpp
SetConsoleCP(CP_UTF8);
SetConsoleOutputCP(CP_UTF8);

SampleRepository    sampleRepo("data/samples.json");
OrderRepository     orderRepo("data/orders.json");
ProductionQueue     prodQueue("data/production.json");

// View 인스턴스
MainView        mainView;
SampleView      sampleView;
// ...

// Controller 조립
AppController app(sampleRepo, orderRepo, prodQueue, mainView, ...);
app.Run();
```

## 메인 메뉴 구성

```
[1] 시료 관리      — 시료 등록, 목록 조회, 이름 검색
[2] 시료 주문      — 고객 주문 접수 (→ RESERVED)
[3] 주문 승인/거절 — RESERVED 목록 확인 후 승인(→ CONFIRMED/PRODUCING) 또는 거절(→ REJECTED)
[4] 모니터링       — 상태별 주문 수 + 시료별 재고 현황
[5] 생산라인 조회  — 현재 생산 중 + 대기 큐 (FIFO)
[6] 출고 처리      — CONFIRMED 주문 출고 (→ RELEASED)
[0] 종료
```

## 빌드 설정 주의사항

- **`/utf-8` 컴파일러 플래그 필수**: 소스 파일 한글을 UTF-8로 읽도록 강제. 모든 `ItemDefinitionGroup`의 `<AdditionalOptions>`에 설정.
- **`#define NOMINMAX`**: `<windows.h>` 포함 시 `min`/`max` 매크로와 `std::numeric_limits` 충돌 방지. `main.cpp` 최상단에 선언.
- **콘솔 인코딩**: `SetConsoleCP(CP_UTF8)` + `SetConsoleOutputCP(CP_UTF8)` — `main()` 첫 줄에서 호출.
- **`data/` 디렉터리**: 실행 파일 기준 현재 작업 디렉터리에 생성. Visual Studio 디버깅 시 프로젝트 디렉터리(`SampleOrderSystem-jsjason/`)가 기준.

## 참조 PoC

| PoC | 재사용 내용 |
|-----|------------|
| [ConsoleMVC-jsjason](https://github.com/jsjason/ConsoleMVC-jsjason) | MVC 레이어 분리 구조, 생성자 주입 패턴 |
| [DataPersistence-jsjason](https://github.com/jsjason/DataPersistence-jsjason) | `json.h` 헤더, Repository 자동 저장 패턴 |
| [DataMonitor-jsjason](https://github.com/jsjason/DataMonitor-jsjason) | 대시보드 렌더링 패턴 (모니터링 화면) |
| [DummyDataGenerator-jsjason](https://github.com/jsjason/DummyDataGenerator-jsjason) | 초기 테스트 데이터 (`samples.json` 시드) |
