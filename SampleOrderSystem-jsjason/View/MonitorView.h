#pragma once
#include <vector>
#include <string>

enum class StockLevel { AMPLE, SHORT, EMPTY };

struct SampleStockStatus {
    std::string sampleId;
    std::string sampleName;
    int         stock;
    StockLevel  level;
};

struct OrderStats {
    int reserved  = 0;
    int confirmed = 0;
    int producing = 0;
    int released  = 0;
};

class MonitorView {
public:
    void showDashboard(const OrderStats&                      stats,
                       const std::vector<SampleStockStatus>& stockStatuses) const;
};
