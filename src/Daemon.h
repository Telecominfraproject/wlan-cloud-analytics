//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#pragma once

#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

#include "Dashboard.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"
#include "framework/MicroService.h"
#include "framework/MicroServiceNames.h"
#include "framework/OpenWifiTypes.h"

namespace OpenWifi {

	[[maybe_unused]] inline static const char *vDAEMON_PROPERTIES_FILENAME =
		"owanalytics.properties";
	[[maybe_unused]] inline static const char *vDAEMON_ROOT_ENV_VAR = "OWANALYTICS_ROOT";
	[[maybe_unused]] inline static const char *vDAEMON_CONFIG_ENV_VAR = "OWANALYTICS_CONFIG";
	[[maybe_unused]] inline static const char *vDAEMON_APP_NAME = uSERVICE_ANALYTICS.c_str();
	[[maybe_unused]] inline static const uint64_t vDAEMON_BUS_TIMER = 10000;

	class Daemon : public MicroService {
	  public:
		explicit Daemon(const std::string &PropFile, const std::string &RootEnv,
						const std::string &ConfigEnv, const std::string &AppName, uint64_t BusTimer,
						const SubSystemVec &SubSystems)
			: MicroService(PropFile, RootEnv, ConfigEnv, AppName, BusTimer, SubSystems){};

		void PostInitialization(Poco::Util::Application &self);
		static Daemon *instance();
		inline OpenWifi::AnalyticsDashboard &GetDashboard() { return DB_; }
		Poco::Logger &Log() { return Poco::Logger::get(AppName()); }

	  private:
		static Daemon *instance_;
		OpenWifi::AnalyticsDashboard DB_{};
	};

	inline Daemon *Daemon() { return Daemon::instance(); }
	void DaemonPostInitialization(Poco::Util::Application &self);
} // namespace OpenWifi
