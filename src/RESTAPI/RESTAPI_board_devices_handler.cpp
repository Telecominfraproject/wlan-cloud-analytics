//
// Created by stephane bourque on 2022-03-14.
//

#include "RESTAPI_board_devices_handler.h"
#include "StorageService.h"
#include "VenueCoordinator.h"

namespace OpenWifi {
	void RESTAPI_board_devices_handler::DoGet() {

		auto id = GetBinding("id", "");

		if (id.empty()) {
			return BadRequest(RESTAPI::Errors::MissingUUID);
		}

		AnalyticsObjects::BoardInfo BI;
		if (!StorageService()->BoardsDB().GetRecord("id", id, BI)) {
			return NotFound();
		}

		AnalyticsObjects::DeviceInfoList DIL;
		VenueCoordinator()->GetDevices(id, DIL);

		Poco::JSON::Object Answer;
		DIL.to_json(Answer);
		return ReturnObject(Answer);
	}
} // namespace OpenWifi