#include <mutex>
#include <map>
#include <string>

namespace OpenWifi {

    class SSID_DICT {
    public:
        static auto instance() {
            static auto instance_ = new SSID_DICT;
            return instance_;
        }

        inline uint64_t Add(const std::string &ssid) {
            std::lock_guard G(Mutex_);
            auto it = Dict_.find(ssid);
            if (it == end(Dict_)) {
                auto Id = Index_++;
                Dict_[ssid] = Id;
                return Id;
            }
            return it->second;
        }

        inline void Remove(const std::string &ssid) {
            std::lock_guard G(Mutex_);
            Dict_.erase(ssid);
        }

        inline std::string Get(uint64_t ssid_id) {
            std::lock_guard G(Mutex_);
            for(const auto &[name,id]:Dict_) {
                if(ssid_id==id)
                    return name;
            }
            return "";
        }

    private:
        std::recursive_mutex            Mutex_;
        uint64_t                        Index_=1;
        std::map<std::string,uint64_t>  Dict_;
    };

    inline auto SSID_DICT() { return SSID_DICT::instance(); }
}