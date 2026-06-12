#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../json.h"

struct Sample {
    std::string id;
    std::string name;
    double      avgProductionTime;  // min/ea
    double      yield;              // 0.0~1.0
    int         stock;              // ea

    nlohmann::json toJson() const;
    static Sample  fromJson(const nlohmann::json& j);
};

class SampleRepository {
public:
    // filePath가 빈 문자열이면 파일 I/O 없이 in-memory로 동작 (테스트용).
    explicit SampleRepository(const std::string& filePath);

    bool                        add(const Sample& sample);
    std::vector<Sample>         getAll() const;
    std::optional<Sample>       findById(const std::string& id) const;
    std::vector<Sample>         searchByName(const std::string& keyword) const;
    bool                        deductStock(const std::string& id, int quantity);
    void                        addStock(const std::string& id, int quantity);
    void                        replaceAll(const std::vector<Sample>& samples);

private:
    std::string         filePath_;
    std::vector<Sample> samples_;

    void load();
    void save();
};
