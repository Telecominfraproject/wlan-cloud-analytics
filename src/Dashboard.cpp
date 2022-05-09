//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "Dashboard.h"
#include "StorageService.h"

namespace OpenWifi {
	void AnalyticsDashboard::Create() {
		uint64_t Now = OpenWifi::Now();
		if(LastRun_==0 || (Now-LastRun_)>120) {
			DB_.reset();
			//  Todo: call dashboard creation code.
			LastRun_ = Now;
		}
	}
}
