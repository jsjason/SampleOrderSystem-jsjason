# SampleOrderSystem-jsjason

가상의 반도체 회사 "S-Semi"의 시료(Sample) 생산주문관리 시스템.
콘솔 기반 C++ 애플리케이션으로, MVC 패턴 + JSON 파일 영속성을 사용한다.

> 기능 명세 및 도메인 모델 상세: [docs/PRD.md](docs/PRD.md)
> Phase별 설계 문서: [docs/design/](docs/design/) — phase1~4b 각 폴더에 model.md / mvc.md 수록

---

## 개발 환경

- **IDE**: Visual Studio 2022 (v18)
- **언어**: C++20
- **플랫폼**: Windows x64 (Debug / Release)
- **툴체인**: MSVC v145
- **빌드**: MSBuild

### 빌드 명령

```powershell
$msbuild = &"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe `
    | Select-Object -First 1
& $msbuild SampleOrderSystem-jsjason\SampleOrderSystem-jsjason.vcxproj `
    /p:Configuration=Debug /p:Platform=x64 /t:Rebuild /nologo /v:minimal
```

빌드 결과물: `SampleOrderSystem-jsjason\x64\Debug\SampleOrderSystem-jsjason.exe`

### 테스트 실행

Debug 빌드를 실행하면 자동으로 GTest 테스트가 수행된다.

```powershell
& SampleOrderSystem-jsjason\x64\Debug\SampleOrderSystem-jsjason.exe
```

---

## 프로젝트 구조

```
SampleOrderSystem-jsjason/          ← git 저장소 루트
├── CLAUDE.md
├── docs/
│   ├── PRD.md                      ← 기능 명세 / 도메인 모델
│   └── design/                     ← Phase별 설계 문서
├── SampleOrderSystem-jsjason.slnx  ← Visual Studio 솔루션
└── SampleOrderSystem-jsjason/      ← C++ 프로젝트 디렉터리
    ├── main.cpp                    ← 진입점. Debug: 테스트 실행 / Release: 앱 실행
    ├── json.h                      ← 헤더 전용 JSON 파서/직렬화기
    ├── Model/                      ← 엔티티 구조체 + Repository (Sample, Order, ProductionQueue)
    ├── View/                       ← 콘솔 렌더링 + 입력 수집 (도메인별 View + ConsoleColor.h)
    ├── Controller/                 ← 비즈니스 로직 (AppController + 도메인별 Controller)
    └── Tests/                      ← GTest 단위 테스트 (48개)
```

---

## MVC 레이어 역할

### Model

데이터 구조 + 저장소 로직만 담당. View/Controller를 전혀 참조하지 않는다.

- `Sample`, `Order`, `ProductionJob` 구조체: `toJson()` / `fromJson()` 포함
- `SampleRepository`, `OrderRepository`, `ProductionQueue`: 생성자 로드, 변경 시 자동 저장

### View

콘솔 렌더링과 사용자 입력 수집만 담당. 비즈니스 로직 없음.

- `show*()` 메서드: 화면 출력
- `prompt*()` 메서드: 사용자 입력 수집

### Controller

Model과 View를 연결. 입력 → 비즈니스 로직 → 결과 표시.

- `AppController::run()` — 최상위 메뉴 루프
- 각 도메인 Controller가 하위 메뉴 루프 처리

### main.cpp

세 레이어를 조립하고 실행. 의존성은 생성자 참조 주입.

```cpp
// Debug: GTest 실행
::testing::InitGoogleTest(&argc, argv);
return RUN_ALL_TESTS();

// Release: 앱 실행
SampleRepository     sampleRepo("data/samples.json");
OrderRepository      orderRepo("data/orders.json");
ProductionQueue      prodQueue("data/production.json");

SampleView           sampleView;
SampleController     sampleCtrl(sampleRepo, orderRepo, prodQueue, sampleView);

OrderView            orderView;
OrderController      orderCtrl(sampleRepo, orderRepo, prodQueue, orderView);

ProductionView       productionView;
ProductionController productionCtrl(sampleRepo, orderRepo, prodQueue, productionView);

ReleaseView          releaseView;
ReleaseController    releaseCtrl(sampleRepo, orderRepo, releaseView);

MonitorView          monitorView;
MonitorController    monitorCtrl(sampleRepo, orderRepo, prodQueue, monitorView);

MainView             mainView;
AppController        app(sampleRepo, orderRepo, prodQueue,
                         sampleCtrl, orderCtrl, productionCtrl,
                         releaseCtrl, monitorCtrl, mainView);
app.run();
```

---

## 빌드 설정 주의사항

- **`/utf-8` 컴파일러 플래그**: 모든 구성의 `<AdditionalOptions>`에 설정. 한글 소스 파일 UTF-8 처리.
- **`NOMINMAX` 전처리기 정의**: `<windows.h>` 포함 시 `min`/`max` 매크로 충돌 방지.
- **콘솔 인코딩**: `SetConsoleCP(CP_UTF8)` + `SetConsoleOutputCP(CP_UTF8)` — `main()` 첫 줄.
- **`data/` 디렉터리**: Visual Studio 디버깅 시 프로젝트 디렉터리(`SampleOrderSystem-jsjason/`)가 기준.

---

## 참조 PoC

| PoC | 재사용 내용 |
|-----|------------|
| [ConsoleMVC-jsjason](https://github.com/jsjason/ConsoleMVC-jsjason) | MVC 레이어 분리 구조, 생성자 주입 패턴 |
| [DataPersistence-jsjason](https://github.com/jsjason/DataPersistence-jsjason) | `json.h` 헤더, Repository 자동 저장 패턴 |
| [DataMonitor-jsjason](https://github.com/jsjason/DataMonitor-jsjason) | 대시보드 렌더링 패턴 (모니터링 화면) |
| [DummyDataGenerator-jsjason](https://github.com/jsjason/DummyDataGenerator-jsjason) | 초기 테스트 데이터 (`samples.json` 시드) |
