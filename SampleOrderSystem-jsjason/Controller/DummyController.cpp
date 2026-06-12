#include "DummyController.h"

DummyController::DummyController(SampleRepository& sampleRepo,
                                 OrderRepository&  orderRepo,
                                 ProductionQueue&  prodQueue,
                                 DummyView&        view)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , view_(view) {}

void DummyController::run() {
    if (!view_.promptConfirm()) return;
    generate();
    view_.showSuccess(5, 11);
}

void DummyController::generate() {
    // ---- 시료 5종 ----
    std::vector<Sample> samples = {
        { "S-001", "실리콘 웨이퍼-8인치", 0.5,  0.92, 480 },
        { "S-002", "GaN 에피택셜-4인치",  1.2,  0.85,   0 },
        { "S-003", "GaAs 웨이퍼-6인치",   0.8,  0.88, 200 },
        { "S-004", "SiC 기판-4인치",       2.0,  0.75, 320 },
        { "S-005", "InP 기판-2인치",       3.5,  0.70,  60 },
    };
    sampleRepo_.replaceAll(samples);

    // ---- 주문 11건 (PRODUCING 제외) ----
    auto makeOrder = [](const std::string& num, const std::string& sampleId,
                        const std::string& customer, int qty,
                        OrderStatus status, const std::string& createdAt) {
        Order o;
        o.orderNumber  = num;
        o.sampleId     = sampleId;
        o.customerName = customer;
        o.quantity     = qty;
        o.status       = status;
        o.createdAt    = createdAt;
        return o;
    };

    std::vector<Order> orders = {
        // RESERVED (3건)
        makeOrder("ORD-20260501-0001","S-001","삼성전자 파운드리",    100, OrderStatus::RESERVED,  "2026-05-01 09:00:00"),
        makeOrder("ORD-20260502-0001","S-003","SK하이닉스 연구소",     80, OrderStatus::RESERVED,  "2026-05-02 10:30:00"),
        makeOrder("ORD-20260503-0001","S-005","한화시스템 연구소",     40, OrderStatus::RESERVED,  "2026-05-03 14:00:00"),
        // CONFIRMED (3건)
        makeOrder("ORD-20260510-0001","S-001","LG이노텍 개발팀",      150, OrderStatus::CONFIRMED, "2026-05-10 09:15:00"),
        makeOrder("ORD-20260511-0001","S-003","한국반도체연구원",      120, OrderStatus::CONFIRMED, "2026-05-11 11:00:00"),
        makeOrder("ORD-20260512-0001","S-004","인텔코리아 팹리스",     200, OrderStatus::CONFIRMED, "2026-05-12 13:45:00"),
        // REJECTED (2건)
        makeOrder("ORD-20260520-0001","S-002","미래나노텍",            100, OrderStatus::REJECTED,  "2026-05-20 10:00:00"),
        makeOrder("ORD-20260521-0001","S-003","서울대학교 물리학과",    50, OrderStatus::REJECTED,  "2026-05-21 16:20:00"),
        // RELEASED (3건)
        makeOrder("ORD-20260601-0001","S-001","삼성전자 파운드리",     200, OrderStatus::RELEASED,  "2026-06-01 09:00:00"),
        makeOrder("ORD-20260602-0001","S-002","SK하이닉스 연구소",      50, OrderStatus::RELEASED,  "2026-06-02 10:00:00"),
        makeOrder("ORD-20260603-0001","S-005","포스코홀딩스 기술원",    30, OrderStatus::RELEASED,  "2026-06-03 11:30:00"),
    };
    orderRepo_.replaceAll(orders);

    // ---- 생산 큐 초기화 ----
    prodQueue_.clear();
}
