#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/DummyView.h"

class DummyController {
public:
    DummyController(SampleRepository& sampleRepo,
                    OrderRepository&  orderRepo,
                    ProductionQueue&  prodQueue,
                    DummyView&        view);
    void run();

private:
    void generate();

    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    DummyView&        view_;
};
