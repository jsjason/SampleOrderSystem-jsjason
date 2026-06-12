#include "AppController.h"

AppController::AppController(SampleRepository&     sampleRepo,
                             OrderRepository&      orderRepo,
                             ProductionQueue&      prodQueue,
                             SampleController&     sampleCtrl,
                             OrderController&      orderCtrl,
                             ProductionController& productionCtrl,
                             ReleaseController&    releaseCtrl,
                             MonitorController&    monitorCtrl,
                             DummyController&      dummyCtrl,
                             MainView&             mainView)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , sampleCtrl_(sampleCtrl)
    , orderCtrl_(orderCtrl)
    , productionCtrl_(productionCtrl)
    , releaseCtrl_(releaseCtrl)
    , monitorCtrl_(monitorCtrl)
    , dummyCtrl_(dummyCtrl)
    , mainView_(mainView) {}

void AppController::run() {
    while (true) {
        prodQueue_.processCompleted(sampleRepo_, orderRepo_);

        const auto samples = sampleRepo_.getAll();
        int sampleCount = static_cast<int>(samples.size());
        int totalStock  = 0;
        for (const auto& s : samples) totalStock += s.stock;
        int orderCount = static_cast<int>(orderRepo_.getAll().size());
        int queueCount = static_cast<int>(prodQueue_.getAll().size());

        mainView_.showMenu(sampleCount, totalStock, orderCount, queueCount);
        int choice = mainView_.promptMenuChoice();

        switch (choice) {
            case 1: sampleCtrl_.run();        break;
            case 2: orderCtrl_.run();         break;
            case 3: orderCtrl_.runApproval(); break;
            case 4: monitorCtrl_.run();       break;
            case 5: productionCtrl_.run();    break;
            case 6: releaseCtrl_.run();       break;
            case 7: dummyCtrl_.run();        break;
            case 0: return;
            default: mainView_.showInvalidInput(); break;
        }
    }
}
