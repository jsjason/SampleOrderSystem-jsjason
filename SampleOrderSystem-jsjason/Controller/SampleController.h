#pragma once
#include "../Model/Sample.h"
#include "../View/SampleView.h"

class SampleController {
public:
    SampleController(SampleRepository& repo, SampleView& view);
    void run();

private:
    void handleRegister();
    void handleList();
    void handleSearch();

    SampleRepository& repo_;
    SampleView&       view_;
};
