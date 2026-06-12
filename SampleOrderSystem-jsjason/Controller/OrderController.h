#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/OrderView.h"

// 승인 처리 결과
struct ApprovalResult {
    OrderStatus newStatus;      // CONFIRMED 또는 PRODUCING
    int         shortage;       // 0이면 재고 충분
    int         actualQuantity; // shortage > 0일 때 실 생산량
};

// 승인 핵심 로직. View 없이 단위 테스트 가능.
ApprovalResult applyApproval(const std::string& orderNumber,
                              SampleRepository&  sampleRepo,
                              OrderRepository&   orderRepo,
                              ProductionQueue&   prodQueue);

// 거절 핵심 로직.
bool applyRejection(const std::string& orderNumber,
                    OrderRepository&   orderRepo);

class OrderController {
public:
    OrderController(SampleRepository& sampleRepo,
                    OrderRepository&  orderRepo,
                    ProductionQueue&  prodQueue,
                    OrderView&        view);

    void run();           // [2] 시료 주문 서브메뉴
    void runApproval();   // [3] 주문 승인/거절 진입점

private:
    void handlePlaceOrder();
    void handleListOrders();
    void handleApprove(const Order& order);
    void handleReject(const Order& order);

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    OrderView&        view_;
};
