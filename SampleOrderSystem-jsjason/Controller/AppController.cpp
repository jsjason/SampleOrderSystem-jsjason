#include "AppController.h"

AppController::AppController(SampleRepository&     sampleRepo,
                             OrderRepository&      orderRepo,
                             ProductionQueue&      prodQueue,
                             SampleController&     sampleCtrl,
                             OrderController&      orderCtrl,
                             ProductionController& productionCtrl,
                             MainView&             mainView)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , sampleCtrl_(sampleCtrl)
    , orderCtrl_(orderCtrl)
    , productionCtrl_(productionCtrl)
    , mainView_(mainView) {}

void AppController::run() {
    while (true) {
        prodQueue_.processCompleted(sampleRepo_, orderRepo_);

        const auto samples = sampleRepo_.getAll();
        int count = static_cast<int>(samples.size());
        int total = 0;
        for (const auto& s : samples) total += s.stock;

        mainView_.showMenu(count, total);
        int choice = mainView_.promptMenuChoice();

        switch (choice) {
            case 1: sampleCtrl_.run();        break;
            case 2: orderCtrl_.run();         break;
            case 3: orderCtrl_.runApproval(); break;
            case 5: productionCtrl_.run();    break;
            case 4: case 6:
                mainView_.showNotImplemented(); break;
            case 0: return;
            default: mainView_.showInvalidInput(); break;
        }
    }
}
