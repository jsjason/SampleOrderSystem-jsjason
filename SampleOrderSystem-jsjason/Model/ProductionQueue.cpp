#include "ProductionQueue.h"
#include "Sample.h"
#include "Order.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

int calculateActualQuantity(int shortage, double yield) {
    if (shortage <= 0) return 0;
    return static_cast<int>(std::ceil(shortage / (yield * 0.9)));
}

// -----------------------------------------------------------------------
// 파일 내부 헬퍼
// -----------------------------------------------------------------------

static std::time_t parseDateTime(const std::string& s) {
    std::tm tm = {};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

static std::string formatDateTime(std::time_t t) {
    std::tm tm = {};
    localtime_s(&tm, &t);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

// -----------------------------------------------------------------------
// ProductionJob
// -----------------------------------------------------------------------

nlohmann::json ProductionJob::toJson() const {
    nlohmann::json j;
    j["orderNumber"]       = orderNumber;
    j["sampleId"]          = sampleId;
    j["actualQuantity"]    = actualQuantity;
    j["enqueuedAt"]        = enqueuedAt;
    j["startedAt"]         = startedAt;
    j["totalDuration"]     = totalDuration;
    j["creditedQuantity"]  = creditedQuantity;
    return j;
}

ProductionJob ProductionJob::fromJson(const nlohmann::json& j) {
    ProductionJob job;
    job.orderNumber       = j.at("orderNumber").get<std::string>();
    job.sampleId          = j.at("sampleId").get<std::string>();
    job.actualQuantity    = j.at("actualQuantity").get<int>();
    job.enqueuedAt        = j.at("enqueuedAt").get<std::string>();
    job.startedAt         = j.at("startedAt").get<std::string>();
    job.totalDuration     = j.at("totalDuration").get<double>();
    try { job.creditedQuantity = j.at("creditedQuantity").get<int>(); }
    catch (...) { job.creditedQuantity = 0; }
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
                               int                actualQuantity,
                               double             avgProductionTime) {
    ProductionJob job;
    job.orderNumber    = orderNumber;
    job.sampleId       = sampleId;
    job.actualQuantity = actualQuantity;
    job.enqueuedAt     = currentDateTimeString();
    job.totalDuration  = avgProductionTime * actualQuantity * 60.0;

    if (jobs_.empty()) {
        job.startedAt = currentDateTimeString();
    } else {
        const auto& last    = jobs_.back();
        std::time_t lastStart = parseDateTime(last.startedAt);
        std::time_t newStart  = lastStart + static_cast<std::time_t>(last.totalDuration);
        job.startedAt = formatDateTime(newStart);
    }

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

void ProductionQueue::processCompleted(SampleRepository& sampleRepo,
                                        OrderRepository&  orderRepo,
                                        std::time_t       now) {
    while (!jobs_.empty()) {
        std::time_t startedAt = parseDateTime(jobs_.front().startedAt);
        double      elapsed   = std::difftime(now, startedAt);

        if (elapsed < 0.0) break;

        // 현재까지 생산된 수량: floor(경과시간 / 개당생산시간)
        int producedSoFar = 0;
        if (jobs_.front().actualQuantity > 0 && jobs_.front().totalDuration > 0.0) {
            double timePerItem = jobs_.front().totalDuration / jobs_.front().actualQuantity;
            producedSoFar = std::min(
                static_cast<int>(elapsed / timePerItem),
                jobs_.front().actualQuantity
            );
        }

        // 이번 호출에서 새로 생산된 수량만 재고에 반영
        int newlyProduced = producedSoFar - jobs_.front().creditedQuantity;
        if (newlyProduced > 0) {
            sampleRepo.addStock(jobs_.front().sampleId, newlyProduced);
            jobs_.front().creditedQuantity += newlyProduced;
            save();
        }

        // 전체 생산 완료 여부 확인
        if (producedSoFar < jobs_.front().actualQuantity) break;

        // 완료 처리: 상태 변경, 큐에서 제거 (재고 차감은 출고 시점에 수행)
        auto order = orderRepo.findByNumber(jobs_.front().orderNumber);
        if (order.has_value()) {
            orderRepo.updateStatus(jobs_.front().orderNumber, OrderStatus::CONFIRMED);
        }
        jobs_.erase(jobs_.begin());
        save();
    }
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
    localtime_s(&tm, &now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
