#include <windows.h>

#ifdef _DEBUG
#include <gtest/gtest.h>
#endif

#ifndef _DEBUG
#include "Model/Sample.h"
#include "Model/Order.h"
#include "Model/ProductionQueue.h"
#include "View/MainView.h"
#include "View/SampleView.h"
#include "View/OrderView.h"
#include "Controller/SampleController.h"
#include "Controller/OrderController.h"
#include "Controller/AppController.h"
#endif

int main(int argc, char* argv[]) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    // ANSI 색상 코드 활성화 (Windows 10+)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

#ifdef _DEBUG
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    SampleRepository  sampleRepo("data/samples.json");
    OrderRepository   orderRepo("data/orders.json");
    ProductionQueue   prodQueue("data/production.json");

    SampleView        sampleView;
    SampleController  sampleCtrl(sampleRepo, sampleView);

    OrderView         orderView;
    OrderController   orderCtrl(sampleRepo, orderRepo, prodQueue, orderView);

    MainView          mainView;
    AppController     app(sampleRepo, sampleCtrl, orderCtrl, mainView);

    app.run();
    return 0;
#endif
}
