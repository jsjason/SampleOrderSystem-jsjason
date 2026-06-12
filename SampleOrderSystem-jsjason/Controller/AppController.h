#pragma once
#include "../Model/Sample.h"
#include "../Model/Order.h"
#include "../Model/ProductionQueue.h"
#include "../View/MainView.h"
#include "SampleController.h"
#include "OrderController.h"
#include "ProductionController.h"
#include "ReleaseController.h"
#include "MonitorController.h"
#include "DummyController.h"

class AppController {
public:
    AppController(SampleRepository&     sampleRepo,
                  OrderRepository&      orderRepo,
                  ProductionQueue&      prodQueue,
                  SampleController&     sampleCtrl,
                  OrderController&      orderCtrl,
                  ProductionController& productionCtrl,
                  ReleaseController&    releaseCtrl,
                  MonitorController&    monitorCtrl,
                  DummyController&      dummyCtrl,
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
    MonitorController&    monitorCtrl_;
    DummyController&      dummyCtrl_;
    MainView&             mainView_;
};
