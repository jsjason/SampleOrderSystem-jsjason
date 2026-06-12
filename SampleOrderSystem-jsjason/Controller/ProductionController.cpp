#include "ProductionController.h"

ProductionController::ProductionController(SampleRepository& sampleRepo,
                                            OrderRepository&  orderRepo,
                                            ProductionQueue&  prodQueue,
                                            ProductionView&   view)
    : sampleRepo_(sampleRepo)
    , orderRepo_(orderRepo)
    , prodQueue_(prodQueue)
    , view_(view) {}

void ProductionController::run() {
    prodQueue_.processCompleted(sampleRepo_, orderRepo_);

    auto jobs = prodQueue_.getAll();
    if (jobs.empty()) {
        view_.showEmpty();
        return;
    }

    std::vector<std::string> sampleNames;
    sampleNames.reserve(jobs.size());
    for (const auto& job : jobs) {
        auto s = sampleRepo_.findById(job.sampleId);
        sampleNames.push_back(s.has_value() ? s->name : job.sampleId);
    }

    view_.showJobList(jobs, sampleNames);
}
