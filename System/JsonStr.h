#pragma once
#include <string>
#include <map>
#include <stdexcept>

class JsonStr {
private:
    std::map<std::string, std::string> data_;  // 存储键值对


    // 内部工具函数：解析字符串到map
    void parseToMap(const std::string& str) {
        size_t pos = 0;
        while (pos < str.size()) {
            size_t key_start = str.find('"', pos);
            if (key_start == std::string::npos) break;
            size_t key_end = str.find('"', key_start + 1);
            if (key_end == std::string::npos) throw std::runtime_error("Invalid JSON: key not closed");
            
            size_t val_start = str.find('"', key_end + 1);
            if (val_start == std::string::npos) throw std::runtime_error("Invalid JSON: value not opened");
            size_t val_end = str.find('"', val_start + 1);
            if (val_end == std::string::npos) throw std::runtime_error("Invalid JSON: value not closed");

            std::string key = str.substr(key_start + 1, key_end - key_start - 1);
            std::string value = str.substr(val_start + 1, val_end - val_start - 1);
            data_[key] = value;
            pos = val_end + 1;
        }
    }

    // 内部工具函数：从map生成字符串
    std::string mapToString() const {
        std::string result;
        bool first = true;
        for (const auto& pair : data_) {
            if (!first) result += ",";
            result += "\"" + pair.first + "\":\"" + pair.second + "\"";
            first = false;
        }
        return result;
    }

public:
    // 构造函数：支持空初始化或从字符串解析
    JsonStr() = default;
    explicit JsonStr(const std::string& json_str) {
        parseToMap(json_str);
    }

    // 增/改：设置键值对
    template<typename T>
    void set(const std::string& key, const T& value) {
        data_[key] = value;
    }

    // 删：删除键
    void remove(const std::string& key) {
        data_.erase(key);
    }

    // 查：获取值（若键不存在返回空字符串或抛出异常）
    std::string get(const std::string& key, bool throw_if_missing = false) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
            if (throw_if_missing) throw std::runtime_error("Key not found: " + key);
            return "";
        }
        return it->second;
    }

    // 序列化为不带大括号的JSON字符串
    std::string toString() const {
        return mapToString();
    }

    // 检查键是否存在
    bool has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    // 清空所有数据
    void clear() {
        data_.clear();
    }
};