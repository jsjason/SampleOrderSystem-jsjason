#pragma once
#include <string>
#include <vector>
#include "../Model/Sample.h"

class SampleView {
public:
    void showMenu() const;
    int  promptMenuChoice() const;

    Sample      promptRegisterInput() const;
    std::string promptSearchKeyword() const;

    void showList(const std::vector<Sample>& samples) const;
    void showSearchResult(const std::vector<Sample>& samples,
                          const std::string& keyword) const;
    void showRegisterSuccess(const Sample& s) const;
    void showRegisterFail(const std::string& reason) const;
    void showEmpty() const;
    void showInvalidInput() const;

private:
    void printTable(const std::vector<Sample>& samples) const;
};
