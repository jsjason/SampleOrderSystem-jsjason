#pragma once
#include "../Model/Sample.h"
#include "../View/MainView.h"
#include "SampleController.h"
#include "OrderController.h"

class AppController {
public:
    AppController(SampleRepository& sampleRepo,
                  SampleController& sampleCtrl,
                  OrderController&  orderCtrl,
                  MainView&         mainView);
    void run();

private:
    SampleRepository& sampleRepo_;
    SampleController& sampleCtrl_;
    OrderController&  orderCtrl_;
    MainView&         mainView_;
};
