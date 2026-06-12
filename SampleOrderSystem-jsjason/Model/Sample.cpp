#include "Sample.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

// ---- Sample ----

nlohmann::json Sample::toJson() const {
    nlohmann::json j;
    j["id"]                = id;
    j["name"]              = name;
    j["avgProductionTime"] = avgProductionTime;
    j["yield"]             = yield;
    j["stock"]             = stock;
    return j;
}

Sample Sample::fromJson(const nlohmann::json& j) {
    Sample s;
    s.id                = j.at("id").get<std::string>();
    s.name              = j.at("name").get<std::string>();
    s.avgProductionTime = j.at("avgProductionTime").get<double>();
    s.yield             = j.at("yield").get<double>();
    s.stock             = j.at("stock").get<int>();
    return s;
}

// ---- SampleRepository ----

SampleRepository::SampleRepository(const std::string& filePath)
    : filePath_(filePath) {
    load();
}

bool SampleRepository::add(const Sample& sample) {
    for (const auto& s : samples_) {
        if (s.id == sample.id) return false;
    }
    samples_.push_back(sample);
    save();
    return true;
}

std::vector<Sample> SampleRepository::getAll() const {
    return samples_;
}

std::optional<Sample> SampleRepository::findById(const std::string& id) const {
    for (const auto& s : samples_) {
        if (s.id == id) return s;
    }
    return std::nullopt;
}

std::vector<Sample> SampleRepository::searchByName(const std::string& keyword) const {
    auto toLower = [](std::string str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return str;
    };
    const std::string kw = toLower(keyword);
    std::vector<Sample> result;
    for (const auto& s : samples_) {
        if (toLower(s.name).find(kw) != std::string::npos)
            result.push_back(s);
    }
    return result;
}

bool SampleRepository::deductStock(const std::string& id, int quantity) {
    for (auto& s : samples_) {
        if (s.id == id) {
            if (s.stock < quantity) return false;
            s.stock -= quantity;
            save();
            return true;
        }
    }
    return false;
}

void SampleRepository::addStock(const std::string& id, int quantity) {
    for (auto& s : samples_) {
        if (s.id == id) {
            s.stock += quantity;
            save();
            return;
        }
    }
}

void SampleRepository::replaceAll(const std::vector<Sample>& samples) {
    samples_ = samples;
    save();
}

void SampleRepository::load() {
    if (filePath_.empty()) return;
    std::ifstream file(filePath_);
    if (!file.is_open()) return;
    try {
        auto arr = nlohmann::json::parse(file);
        for (const auto& item : arr)
            samples_.push_back(Sample::fromJson(item));
    } catch (...) {
        samples_.clear();
    }
}

void SampleRepository::save() {
    if (filePath_.empty()) return;
    const auto parent = std::filesystem::path(filePath_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    auto arr = nlohmann::json::array();
    for (const auto& s : samples_)
        arr.push_back(s.toJson());
    std::ofstream file(filePath_);
    file << arr.dump(2);
}
