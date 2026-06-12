#include "SampleController.h"

SampleController::SampleController(SampleRepository& repo, SampleView& view)
    : repo_(repo), view_(view) {}

void SampleController::run() {
    while (true) {
        view_.showMenu();
        int choice = view_.promptMenuChoice();
        switch (choice) {
            case 1: handleRegister(); break;
            case 2: handleList();     break;
            case 3: handleSearch();   break;
            case 0: return;
            default: view_.showInvalidInput(); break;
        }
    }
}

void SampleController::handleRegister() {
    Sample s = view_.promptRegisterInput();
    if (repo_.add(s))
        view_.showRegisterSuccess(s);
    else
        view_.showRegisterFail("이미 등록된 시료 ID입니다: " + s.id);
}

void SampleController::handleList() {
    auto samples = repo_.getAll();
    if (samples.empty())
        view_.showEmpty();
    else
        view_.showList(samples);
}

void SampleController::handleSearch() {
    std::string keyword = view_.promptSearchKeyword();
    auto results = repo_.searchByName(keyword);
    view_.showSearchResult(results, keyword);
}
