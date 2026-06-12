#include "Order.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

// ---- 문자열 변환 헬퍼 ----

static std::string statusToString(OrderStatus s) {
    switch (s) {
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::RELEASED:  return "RELEASED";
        default:                     return "RESERVED";
    }
}

static OrderStatus statusFromString(const std::string& s) {
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "RELEASED")  return OrderStatus::RELEASED;
    return OrderStatus::RESERVED;
}

// ---- Order ----

nlohmann::json Order::toJson() const {
    nlohmann::json j;
    j["orderNumber"]  = orderNumber;
    j["sampleId"]     = sampleId;
    j["customerName"] = customerName;
    j["quantity"]     = quantity;
    j["status"]       = statusToString(status);
    j["createdAt"]    = createdAt;
    return j;
}

Order Order::fromJson(const nlohmann::json& j) {
    Order o;
    o.orderNumber  = j.at("orderNumber").get<std::string>();
    o.sampleId     = j.at("sampleId").get<std::string>();
    o.customerName = j.at("customerName").get<std::string>();
    o.quantity     = j.at("quantity").get<int>();
    o.status       = statusFromString(j.at("status").get<std::string>());
    o.createdAt    = j.at("createdAt").get<std::string>();
    return o;
}

// ---- OrderRepository ----

OrderRepository::OrderRepository(const std::string& filePath)
    : filePath_(filePath), nextSeq_(1) {
    load();
    lastDate_ = currentDateString();
    // 오늘 날짜 기준으로 다음 시퀀스 번호 초기화
    const std::string prefix = "ORD-" + lastDate_ + "-";
    int maxSeq = 0;
    for (const auto& o : orders_) {
        if (o.orderNumber.size() > prefix.size() &&
            o.orderNumber.substr(0, prefix.size()) == prefix) {
            try {
                int seq = std::stoi(o.orderNumber.substr(prefix.size()));
                if (seq > maxSeq) maxSeq = seq;
            } catch (...) {}
        }
    }
    nextSeq_ = maxSeq + 1;
}

std::string OrderRepository::currentDateString() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[9];
    std::strftime(buf, sizeof(buf), "%Y%m%d", &tm);
    return std::string(buf);
}

std::string OrderRepository::currentDateTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string OrderRepository::generateOrderNumber() {
    const std::string today = currentDateString();
    if (today != lastDate_) {
        lastDate_ = today;
        nextSeq_  = 1;
    }
    std::ostringstream oss;
    oss << "ORD-" << today << "-"
        << std::setw(4) << std::setfill('0') << nextSeq_++;
    return oss.str();
}

Order OrderRepository::add(const std::string& sampleId,
                           const std::string& customerName,
                           int quantity) {
    Order o;
    o.orderNumber  = generateOrderNumber();
    o.sampleId     = sampleId;
    o.customerName = customerName;
    o.quantity     = quantity;
    o.status       = OrderStatus::RESERVED;
    o.createdAt    = currentDateTimeString();
    orders_.push_back(o);
    save();
    return o;
}

std::vector<Order> OrderRepository::getAll() const {
    return orders_;
}

std::optional<Order> OrderRepository::findByNumber(const std::string& orderNumber) const {
    for (const auto& o : orders_) {
        if (o.orderNumber == orderNumber) return o;
    }
    return std::nullopt;
}

std::vector<Order> OrderRepository::filterByStatus(OrderStatus status) const {
    std::vector<Order> result;
    for (const auto& o : orders_) {
        if (o.status == status) result.push_back(o);
    }
    return result;
}

bool OrderRepository::updateStatus(const std::string& orderNumber, OrderStatus newStatus) {
    for (auto& o : orders_) {
        if (o.orderNumber == orderNumber) {
            o.status = newStatus;
            save();
            return true;
        }
    }
    return false;
}

void OrderRepository::replaceAll(const std::vector<Order>& orders) {
    orders_ = orders;
    save();
}

void OrderRepository::load() {
    if (filePath_.empty()) return;
    std::ifstream file(filePath_);
    if (!file.is_open()) return;
    try {
        auto arr = nlohmann::json::parse(file);
        for (const auto& item : arr)
            orders_.push_back(Order::fromJson(item));
    } catch (...) {
        orders_.clear();
    }
}

void OrderRepository::save() {
    if (filePath_.empty()) return;
    const auto parent = std::filesystem::path(filePath_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    auto arr = nlohmann::json::array();
    for (const auto& o : orders_)
        arr.push_back(o.toJson());
    std::ofstream file(filePath_);
    file << arr.dump(2);
}
