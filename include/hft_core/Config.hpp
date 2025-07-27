#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <variant>
#include <vector>

namespace hft::core {

using ConfigValue = std::variant<std::string, int, double, bool>;

class Config {
public:
    static Config& instance() {
        static Config config;
        return config;
    }

    bool load_from_file(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            // Remove quotes if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            config_map_[key] = parse_value(value);
        }

        return true;
    }

    template<typename T>
    T get(const std::string& key, const T& default_value = T{}) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = config_map_.find(key);
        if (it == config_map_.end()) {
            return default_value;
        }

        try {
            return std::get<T>(it->second);
        } catch (const std::bad_variant_access&) {
            return default_value;
        }
    }

    template<typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_map_[key] = value;
    }

    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_map_.find(key) != config_map_.end();
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_map_.erase(key);
    }

    std::vector<std::string> get_keys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(config_map_.size());
        
        for (const auto& pair : config_map_) {
            keys.push_back(pair.first);
        }
        
        return keys;
    }

    bool save_to_file(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        for (const auto& [key, value] : config_map_) {
            file << key << "=";
            
            std::visit([&file](const auto& v) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                    file << "\"" << v << "\"";
                } else {
                    file << v;
                }
            }, value);
            
            file << "\n";
        }

        return true;
    }

private:
    Config() = default;

    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    ConfigValue parse_value(const std::string& value) {
        // Try to parse as boolean
        if (value == "true" || value == "TRUE") {
            return true;
        }
        if (value == "false" || value == "FALSE") {
            return false;
        }

        // Try to parse as integer
        try {
            size_t pos;
            int int_val = std::stoi(value, &pos);
            if (pos == value.length()) {
                return int_val;
            }
        } catch (...) {}

        // Try to parse as double
        try {
            size_t pos;
            double double_val = std::stod(value, &pos);
            if (pos == value.length()) {
                return double_val;
            }
        } catch (...) {}

        // Default to string
        return value;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ConfigValue> config_map_;
};

// Convenience macros for configuration access
#define CONFIG_GET_STRING(key, default_val) hft::core::Config::instance().get<std::string>(key, default_val)
#define CONFIG_GET_INT(key, default_val) hft::core::Config::instance().get<int>(key, default_val)
#define CONFIG_GET_DOUBLE(key, default_val) hft::core::Config::instance().get<double>(key, default_val)
#define CONFIG_GET_BOOL(key, default_val) hft::core::Config::instance().get<bool>(key, default_val)

#define CONFIG_SET(key, value) hft::core::Config::instance().set(key, value)

} // namespace hft::core