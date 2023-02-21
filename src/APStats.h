//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include "Poco/Logger.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"
#include "framework/utils.h"
#include "nlohmann/json.hpp"
#include <mutex>

namespace OpenWifi {

	struct InterfaceClientEntry {
		std::vector<std::string> ipv4_addresses;
		std::vector<std::string> ipv6_addresses;
	};

	using InterfaceClientEntryMap_t = std::map<std::string, InterfaceClientEntry>;

	class AP {
	  public:
		explicit AP(uint64_t mac, const std::string &venue_id, const std::string &BoardId,
					Poco::Logger &L)
			: venue_id_(venue_id), boardId_(BoardId), Logger_(L) {
			DI_.serialNumber = Utils::IntToSerialNumber(mac);
		}

		void UpdateStats(const std::shared_ptr<nlohmann::json> &State);
		void UpdateConnection(const std::shared_ptr<nlohmann::json> &Connection);
		void UpdateHealth(const std::shared_ptr<nlohmann::json> &Health);

		[[nodiscard]] const AnalyticsObjects::DeviceInfo &Info() const { return DI_; }

	  private:
		std::string venue_id_;
		std::string boardId_;
		AnalyticsObjects::DeviceInfo DI_;
		AnalyticsObjects::DeviceTimePoint tp_base_;
		bool got_health = false, got_connection = false, got_base = false;
		Poco::Logger &Logger_;
		inline Poco::Logger &Logger() { return Logger_; }
	};
} // namespace OpenWifi
