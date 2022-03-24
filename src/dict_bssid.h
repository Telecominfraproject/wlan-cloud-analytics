//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include <mutex>
#include <map>

namespace OpenWifi {
    class BSSID_DICT {
    public:
        static auto instance() {
            static auto instance_ = new BSSID_DICT;
            return instance_;
        }

        inline uint32_t Get(uint64_t bssid) {
            std::lock_guard G(Mutex_);
            auto it = Dict_.find(bssid);
            if(it==end(Dict_)) {
                auto I = Index_++;
                Dict_[bssid]=I;
                return I;
            } else {
                return it->second;
            }
        }

        inline void Remove(uint64_t bssid) {
            std::lock_guard G(Mutex_);
            Dict_.erase(bssid);
        }

    private:
        uint32_t                    Index_=1;
        std::mutex                  Mutex_;
        std::map<uint64_t,uint32_t> Dict_;
    };
    inline auto BSSID_DICT() { return BSSID_DICT::instance(); }
}