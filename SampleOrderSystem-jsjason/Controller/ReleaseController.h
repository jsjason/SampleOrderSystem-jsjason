#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../View/ReleaseView.h"

class ReleaseController {
public:
    ReleaseController(SampleRepository& sampleRepo,
                      OrderRepository&  orderRepo,
                      ReleaseView&      view);
    void run();

private:
    SampleRepository& sampleRepo_;
    OrderRepository&  orderRepo_;
    ReleaseView&      view_;

    void handleRelease(const Order& order, const std::string& sampleName);
};
