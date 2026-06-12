#pragma once
#include "../Model/Sample.h"
#include "../View/MainView.h"
#include "SampleController.h"

class AppController {
public:
    AppController(SampleRepository& sampleRepo,
                  SampleController& sampleCtrl,
                  MainView&         mainView);
    void run();

private:
    SampleRepository& sampleRepo_;
    SampleController& sampleCtrl_;
    MainView&         mainView_;
};
