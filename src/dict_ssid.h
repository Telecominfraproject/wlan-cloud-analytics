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

        inline uint32_t Add(const std::string &ssid) {
            std::lock_guard G(Mutex_);
            auto it = Dict_.find(ssid);
            if (it == end(Dict_)) {
                auto Id = Index_++;
                Dict_[station] = Id;
                return Id;
            } else {
                return it->second;
            }
        }

        inline void Remove(const std::string &ssid) {
            std::lock_guard G(Mutex_);
            Dict_.erase(ssid);
        }

    private:
        std::mutex                      Mutex_;
        uint32_t                        Index_=1;
        std::map<std::string,uint32_t>  Dict_;
    };

    inline auto SSID_DICT() { return SSID_DICT::instance(); }
}