//
// Created by stephane bourque on 2021-08-11.
//

#include <mutex>

#include "StorageService.h"
#include "WifiClientCache.h"
#include "fmt/format.h"
#include "framework/utils.h"

namespace OpenWifi {

	int WifiClientCache::Start() {
		poco_notice(Logger(), "Starting...");
		TimerCallback_ = std::make_unique<Poco::TimerCallback<WifiClientCache>>(
			*this, &WifiClientCache::onTimer);
		Timer_.setStartInterval(30 * 1000);			// first run in 20 seconds
		Timer_.setPeriodicInterval(60 * 60 * 1000); // 1 hours
		Timer_.start(*TimerCallback_);
		return 0;
	}

	void WifiClientCache::Stop() {
		poco_notice(Logger(), "Stopping...");
		Timer_.stop();
		poco_notice(Logger(), "Stopped...");
	}

	void WifiClientCache::onTimer([[maybe_unused]] Poco::Timer &timer) {
		std::vector<std::pair<std::string, std::string>> WifiClients;
		if (StorageService()->WifiClientHistoryDB().GetClientMacs(WifiClients)) {
			//  Let's replace current cache...
			std::lock_guard G(Mutex_);
			Cache_.clear();
			for (const auto &mac : WifiClients)
				AddSerialNumber(mac.second, mac.first, G);
		}
	}

	void WifiClientCache::AddSerialNumber(const std::string &venue_id, const std::string &S) {
		std::lock_guard G(Mutex_);
		AddSerialNumber(venue_id, S, G);
	}

	void
	WifiClientCache::AddSerialNumber(const std::string &venue_id, const std::string &S,
									 [[maybe_unused]] std::lock_guard<std::recursive_mutex> &G) {
		auto VenueIt = Cache_.find(venue_id);
		if (VenueIt == Cache_.end()) {
			Cache_.insert(std::pair(venue_id, Cache{}));
			VenueIt = Cache_.find(venue_id);
		}

		uint64_t SN = std::stoull(S, nullptr, 16);
		if (std::find(std::begin(VenueIt->second.SNs_), std::end(VenueIt->second.SNs_), SN) ==
			std::end(VenueIt->second.SNs_)) {
			auto insert_point =
				std::lower_bound(VenueIt->second.SNs_.begin(), VenueIt->second.SNs_.end(), SN);
			VenueIt->second.SNs_.insert(insert_point, SN);

			auto R = ReverseSerialNumber(S);
			uint64_t RSN = std::stoull(R, nullptr, 16);
			auto rev_insert_point = std::lower_bound(VenueIt->second.Reverse_SNs_.begin(),
													 VenueIt->second.Reverse_SNs_.end(), RSN);
			VenueIt->second.Reverse_SNs_.insert(rev_insert_point, RSN);
		}
	}

	void WifiClientCache::DeleteSerialNumber(const std::string &venue_id, const std::string &S) {
		std::lock_guard G(Mutex_);

		uint64_t SN = std::stoull(S, nullptr, 16);

		auto VenueIt = Cache_.find(venue_id);
		if (VenueIt == Cache_.end())
			return;
		auto It = std::find(VenueIt->second.SNs_.begin(), VenueIt->second.SNs_.end(), SN);
		if (It != VenueIt->second.SNs_.end()) {
			VenueIt->second.SNs_.erase(It);

			auto R = ReverseSerialNumber(S);
			uint64_t RSN = std::stoull(R, nullptr, 16);
			auto RIt = std::find(VenueIt->second.Reverse_SNs_.begin(),
								 VenueIt->second.Reverse_SNs_.end(), RSN);
			if (RIt != VenueIt->second.Reverse_SNs_.end()) {
				VenueIt->second.Reverse_SNs_.erase(RIt);
			}
		}
	}

	uint64_t Reverse(uint64_t N) {
		uint64_t Res = 0;

		for (int i = 0; i < 16; i++) {
			Res = (Res << 4) + (N & 0x000000000000000f);
			N >>= 4;
		}
		Res >>= 16;
		return Res;
	}

	void WifiClientCache::ReturnNumbers(const std::string &S, uint HowMany,
										const std::vector<uint64_t> &SNArr,
										std::vector<uint64_t> &A, bool ReverseResult) {
		std::lock_guard G(Mutex_);

		if (S.length() == 12) {
			uint64_t SN = std::stoull(S, nullptr, 16);
			auto It = std::find(SNArr.begin(), SNArr.end(), SN);
			if (It != SNArr.end()) {
				A.push_back(ReverseResult ? Reverse(*It) : *It);
			}
		} else if (S.length() < 12) {
			std::string SS{S};
			SS.insert(SS.end(), 12 - SS.size(), '0');
			uint64_t SN = std::stoull(SS, nullptr, 16);
			auto LB = std::lower_bound(SNArr.begin(), SNArr.end(), SN);
			if (LB != SNArr.end()) {
				for (; LB != SNArr.end() && HowMany; ++LB, --HowMany) {
					if (ReverseResult) {
						const auto TSN =
							ReverseSerialNumber(Utils::IntToSerialNumber(Reverse(*LB)));
						if (S == TSN.substr(0, S.size())) {
							A.emplace_back(Reverse(*LB));
						} else {
							break;
						}
					} else {
						const auto TSN = Utils::IntToSerialNumber(*LB);
						if (S == TSN.substr(0, S.size())) {
							A.emplace_back(*LB);
						} else {
							break;
						}
					}
				}
			}
		}
	}

	void WifiClientCache::FindNumbers(const std::string &venueId, const std::string &SerialNumber,
									  std::uint64_t StartingOffset, std::uint64_t HowMany,
									  std::vector<uint64_t> &A) {
		std::lock_guard G(Mutex_);

		A.clear();
		auto VenueIt = Cache_.find(venueId);
		if (VenueIt == Cache_.end())
			return;

		if (SerialNumber.empty()) {
			auto Start = VenueIt->second.SNs_.begin();
			std::uint64_t Offset = 0;
			while (HowMany && Start != VenueIt->second.SNs_.end()) {
				if (Offset >= StartingOffset) {
					A.push_back(*Start);
					HowMany--;
				}
				Start++;
				Offset++;
			}
			return;
		}

		if (SerialNumber[0] == '*') {
			std::string Reversed;
			std::copy(rbegin(SerialNumber), rend(SerialNumber) - 1, std::back_inserter(Reversed));
			if (Reversed.empty())
				return;
			return ReturnNumbers(Reversed, HowMany, VenueIt->second.Reverse_SNs_, A, true);
		} else {
			return ReturnNumbers(SerialNumber, HowMany, VenueIt->second.SNs_, A, false);
		}
	}
} // namespace OpenWifi