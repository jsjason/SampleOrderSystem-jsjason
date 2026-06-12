#include "ProductionQueue.h"
#include <fstream>
#include <filesystem>
#include <ctime>
#include <cmath>

int calculateActualQuantity(int shortage, double yield) {
    return static_cast<int>(std::ceil(shortage / (yield * 0.9)));
}

// -----------------------------------------------------------------------
// ProductionJob
// -----------------------------------------------------------------------

nlohmann::json ProductionJob::toJson() const {
    nlohmann::json j;
    j["orderNumber"]    = orderNumber;
    j["sampleId"]       = sampleId;
    j["actualQuantity"] = actualQuantity;
    j["enqueuedAt"]     = enqueuedAt;
    j["startedAt"]      = startedAt;
    j["totalDuration"]  = totalDuration;
    return j;
}

ProductionJob ProductionJob::fromJson(const nlohmann::json& j) {
    ProductionJob job;
    job.orderNumber    = j.at("orderNumber").get<std::string>();
    job.sampleId       = j.at("sampleId").get<std::string>();
    job.actualQuantity = j.at("actualQuantity").get<int>();
    job.enqueuedAt     = j.at("enqueuedAt").get<std::string>();
    job.startedAt      = j.at("startedAt").get<std::string>();
    job.totalDuration  = j.at("totalDuration").get<double>();
    return job;
}

// -----------------------------------------------------------------------
// ProductionQueue
// -----------------------------------------------------------------------

ProductionQueue::ProductionQueue(const std::string& filePath)
    : filePath_(filePath) {
    load();
}

void ProductionQueue::enqueue(const std::string& orderNumber,
                               const std::string& sampleId,
                               int actualQuantity) {
    ProductionJob job;
    job.orderNumber    = orderNumber;
    job.sampleId       = sampleId;
    job.actualQuantity = actualQuantity;
    job.enqueuedAt     = currentDateTimeString();
    job.startedAt      = "";
    job.totalDuration  = 0.0;
    jobs_.push_back(job);
    save();
}

std::optional<ProductionJob> ProductionQueue::front() const {
    if (jobs_.empty()) return std::nullopt;
    return jobs_.front();
}

void ProductionQueue::dequeue() {
    if (!jobs_.empty()) {
        jobs_.erase(jobs_.begin());
        save();
    }
}

std::vector<ProductionJob> ProductionQueue::getAll() const {
    return jobs_;
}

bool ProductionQueue::empty() const {
    return jobs_.empty();
}

void ProductionQueue::load() {
    if (filePath_.empty()) return;
    std::ifstream f(filePath_);
    if (!f.is_open()) return;
    try {
        auto arr = nlohmann::json::parse(f);
        for (const auto& item : arr)
            jobs_.push_back(ProductionJob::fromJson(item));
    } catch (...) {
        jobs_.clear();
    }
}

void ProductionQueue::save() {
    if (filePath_.empty()) return;
    std::filesystem::create_directories(
        std::filesystem::path(filePath_).parent_path());
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& job : jobs_)
        arr.push_back(job.toJson());
    std::ofstream f(filePath_);
    f << arr.dump(2);
}

std::string ProductionQueue::currentDateTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
