#include "AppController.h"

AppController::AppController(SampleRepository& sampleRepo,
                             SampleController& sampleCtrl,
                             MainView&         mainView)
    : sampleRepo_(sampleRepo)
    , sampleCtrl_(sampleCtrl)
    , mainView_(mainView) {}

void AppController::run() {
    while (true) {
        const auto samples = sampleRepo_.getAll();
        int count = static_cast<int>(samples.size());
        int total = 0;
        for (const auto& s : samples) total += s.stock;

        mainView_.showMenu(count, total);
        int choice = mainView_.promptMenuChoice();

        switch (choice) {
            case 1: sampleCtrl_.run(); break;
            case 2: case 3: case 4: case 5: case 6:
                mainView_.showNotImplemented(); break;
            case 0: return;
            default: mainView_.showInvalidInput(); break;
        }
    }
}
