#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/MainView.h"
#include "SampleController.h"
#include "OrderController.h"
#include "ProductionController.h"
#include "ReleaseController.h"

class AppController {
public:
    AppController(SampleRepository&     sampleRepo,
                  OrderRepository&      orderRepo,
                  ProductionQueue&      prodQueue,
                  SampleController&     sampleCtrl,
                  OrderController&      orderCtrl,
                  ProductionController& productionCtrl,
                  ReleaseController&    releaseCtrl,
                  MainView&             mainView);
    void run();

private:
    SampleRepository&     sampleRepo_;
    OrderRepository&      orderRepo_;
    ProductionQueue&      prodQueue_;
    SampleController&     sampleCtrl_;
    OrderController&      orderCtrl_;
    ProductionController& productionCtrl_;
    ReleaseController&    releaseCtrl_;
    MainView&             mainView_;
};
