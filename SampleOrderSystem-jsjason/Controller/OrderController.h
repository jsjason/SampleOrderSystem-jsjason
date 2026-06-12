#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../View/OrderView.h"

class OrderController {
public:
    OrderController(SampleRepository& sampleRepo,
                    OrderRepository&  orderRepo,
                    OrderView&        view);
    void run();

private:
    void handlePlaceOrder();
    void handleListOrders();

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    OrderView&        view_;
};
