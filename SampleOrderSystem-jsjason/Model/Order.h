#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../json.h"

enum class OrderStatus { RESERVED, CONFIRMED, PRODUCING, REJECTED, RELEASED };

struct Order {
    std::string orderNumber;
    std::string sampleId;
    std::string customerName;
    int         quantity;
    OrderStatus status;
    std::string createdAt;  // "YYYY-MM-DD HH:MM:SS"

    nlohmann::json toJson() const;
    static Order   fromJson(const nlohmann::json& j);
};

class OrderRepository {
public:
    // filePath가 빈 문자열이면 파일 I/O 없이 in-memory로 동작 (테스트용).
    explicit OrderRepository(const std::string& filePath);

    // 주문번호·createdAt 자동 생성. 상태는 RESERVED로 고정.
    Order add(const std::string& sampleId,
              const std::string& customerName,
              int quantity);

    std::vector<Order>         getAll() const;
    std::optional<Order>       findByNumber(const std::string& orderNumber) const;
    std::vector<Order>         filterByStatus(OrderStatus status) const;
    bool                       updateStatus(const std::string& orderNumber, OrderStatus newStatus);
    void                       replaceAll(const std::vector<Order>& orders);

private:
    std::string        filePath_;
    std::vector<Order> orders_;
    std::string        lastDate_;
    int                nextSeq_;

    void        load();
    void        save();
    std::string generateOrderNumber();
    std::string currentDateString();
    std::string currentDateTimeString();
};
