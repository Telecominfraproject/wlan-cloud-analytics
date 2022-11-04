//
// Created by stephane bourque on 2021-08-11.
//

#pragma once

#include "framework/SubSystemServer.h"
#include "Poco/Timer.h"

namespace OpenWifi {
	class WifiClientCache : public SubSystemServer {
		public:

		static auto instance() {
		    static auto instance_ = new WifiClientCache;
			return instance_;
		}

		int Start() override;
		void Stop() override;
		void AddSerialNumber(const std::string &venueId, const std::string &SerialNumber);
		void DeleteSerialNumber(const std::string &venueId, const std::string &SerialNumber);
		void FindNumbers(const std::string &venueId, const std::string &SerialNumber, std::uint64_t start, std::uint64_t HowMany, std::vector<uint64_t> &A);
		inline bool NumberExists(const std::string &venueId, uint64_t SerialNumber) {
			std::lock_guard		G(Mutex_);
            auto It = Cache_.find(venueId);
            if(It==Cache_.end())
                return false;
			return std::find(It->second.SNs_.begin(),It->second.SNs_.end(),SerialNumber)!=It->second.SNs_.end();
		}

		static inline std::string ReverseSerialNumber(const std::string &S) {
			std::string ReversedString;
			std::copy(rbegin(S),rend(S),std::back_inserter(ReversedString));
			return ReversedString;
		}
        void onTimer(Poco::Timer & timer);

	  private:
        struct Cache {
            std::vector<uint64_t>		                            SNs_;
            std::vector<uint64_t>		                            Reverse_SNs_;
        };
        std::map<std::string,Cache>                             Cache_;
        Poco::Timer                                             Timer_;
        std::unique_ptr<Poco::TimerCallback<WifiClientCache>>   TimerCallback_;

        void AddSerialNumber(const std::string &venueId, const std::string &S, std::lock_guard<std::recursive_mutex> & G);

        void ReturnNumbers(const std::string &S, uint HowMany, const std::vector<uint64_t> & SNArr, std::vector<uint64_t> &A, bool ReverseResult);

        WifiClientCache() noexcept:
			SubSystemServer("SerialNumberCache", "SNCACHE-SVR", "serialcache")
			{
			}
	};

	inline auto WifiClientCache() { return WifiClientCache::instance(); }

} // namespace OpenWiFi
