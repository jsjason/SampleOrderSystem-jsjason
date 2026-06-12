#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/ProductionView.h"

class ProductionController {
public:
    ProductionController(SampleRepository& sampleRepo,
                         OrderRepository&  orderRepo,
                         ProductionQueue&  prodQueue,
                         ProductionView&   view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    ProductionView&   view_;
};
