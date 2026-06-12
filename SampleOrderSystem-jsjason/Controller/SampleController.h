#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/SampleView.h"

class SampleController {
public:
    SampleController(SampleRepository& sampleRepo,
                     OrderRepository&  orderRepo,
                     ProductionQueue&  prodQueue,
                     SampleView&       view);
    void run();

private:
    void handleRegister();
    void handleList();
    void handleSearch();

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    SampleView&       view_;
};
