//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//
#pragma once

#include "RESTObjects/RESTAPI_AnalyticsObjects.h"
#include "framework/OpenWifiTypes.h"

namespace OpenWifi {
	class AnalyticsDashboard {
	  public:
		void Create();
		[[nodiscard]] const AnalyticsObjects::Report &Report() const { return DB_; }
		inline void Reset() {
			LastRun_ = 0;
			DB_.reset();
		}

	  private:
		AnalyticsObjects::Report DB_{};
		uint64_t LastRun_ = 0;
	};
} // namespace OpenWifi
