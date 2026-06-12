#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/MonitorView.h"

class MonitorController {
public:
    MonitorController(SampleRepository& sampleRepo,
                      OrderRepository&  orderRepo,
                      ProductionQueue&  prodQueue,
                      MonitorView&      view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ProductionQueue&  prodQueue_;
    MonitorView&      view_;
};
