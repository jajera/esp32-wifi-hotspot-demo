#pragma once

#include <map>
#include <string>

// In-memory NVS mock for host-native property tests (extend as needed).
class MockNvs {
public:
    bool putString(const std::string& ns, const std::string& key, const std::string& value) {
        data_[ns + ":" + key] = value;
        return true;
    }

    std::string getString(const std::string& ns, const std::string& key, const std::string& def = "") const {
        auto it = data_.find(ns + ":" + key);
        return it == data_.end() ? def : it->second;
    }

    bool putBool(const std::string& ns, const std::string& key, bool value) {
        return putString(ns, key, value ? "1" : "0");
    }

    bool getBool(const std::string& ns, const std::string& key, bool def = false) const {
        auto v = getString(ns, key, "");
        if (v.empty()) {
            return def;
        }
        return v == "1";
    }

    void remove(const std::string& ns, const std::string& key) { data_.erase(ns + ":" + key); }

private:
    std::map<std::string, std::string> data_;
};
