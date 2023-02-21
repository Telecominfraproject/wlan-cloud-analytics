//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include <map>
#include <mutex>

namespace OpenWifi {

	class UE_DICT {
	  public:
		static auto instance() {
			static auto instance_ = new UE_DICT;
			return instance_;
		}

		inline void Add(uint64_t station, UE *ue) {
			std::lock_guard G(Mutex_);
			auto it = Dict_.find(station);
			if (it == end(Dict_)) {
				Dict_[station] = ue;
				return;
			} else {
				it->second = ue;
			}
		}

		inline void Remove(uint64_t station) {
			std::lock_guard G(Mutex_);
			Dict_.erase(station);
		}

		inline UE *Get(uint64_t station) {
			std::lock_guard G(Mutex_);
			auto it = Dict_.find(station);
			if (it == end(Dict_))
				return nullptr;
			return it->second;
		}

	  private:
		std::mutex Mutex_;
		std::map<uint64_t, UE *> Dict_;
	};

	inline auto UE_DICT() { return UE_DICT::instance(); }
} // namespace OpenWifi