#pragma once
#include <vector>
#include <string>
#include "../Model/ProductionQueue.h"

class ProductionView {
public:
    void showJobList(const std::vector<ProductionJob>& jobs,
                     const std::vector<std::string>&   sampleNames) const;
    void showEmpty() const;
};
