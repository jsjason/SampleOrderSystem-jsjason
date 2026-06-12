#pragma once
#include <vector>
#include <string>
#include "../Model/Order.h"

class ReleaseView {
public:
    void showConfirmedList(const std::vector<Order>&        orders,
                           const std::vector<std::string>& sampleNames) const;
    void showEmpty()                                                      const;
    void showReleaseSuccess(const Order& order, const std::string& sampleName) const;
    void showInvalidInput()                                               const;
    int  promptOrderSelection(int count)                                  const;
};
