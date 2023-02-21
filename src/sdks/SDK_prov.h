//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include "RESTObjects/RESTAPI_ProvObjects.h"
#include "framework/RESTAPI_Handler.h"

namespace OpenWifi::SDK::Prov {

	namespace Venue {
		bool GetDevices(RESTAPIHandler *client, const std::string &VenueId, bool WithChildren,
						ProvObjects::VenueDeviceList &Devices, bool &VenueExists);
		bool Get(RESTAPIHandler *client, const std::string &VenueId, ProvObjects::Venue &Venue,
				 bool &Exists);
		bool Exists(RESTAPIHandler *client, const std::string &VenueId, bool &Exists);
	} // namespace Venue

	namespace Device {
		bool Get(RESTAPIHandler *client, const std::string &Mac,
				 ProvObjects::InventoryTagList &Device);
		bool SetConfiguration(RESTAPIHandler *client, const std::string &Mac,
							  const std::string &ConfigUUID);
	} // namespace Device

	namespace Configuration {
		bool Get(RESTAPIHandler *client, const std::string &ConfigUUID,
				 ProvObjects::DeviceConfiguration &Config);
		bool Delete(RESTAPIHandler *client, const std::string &ConfigUUID);
		bool Create(RESTAPIHandler *client, const std::string &SerialNumber,
					const ProvObjects::DeviceConfiguration &Config, std::string &ConfigUUID);
		bool Update(RESTAPIHandler *client, const std::string &ConfigUUID,
					ProvObjects::DeviceConfiguration &Config);
	} // namespace Configuration

	namespace Subscriber {
		bool GetDevices(RESTAPIHandler *client, const std::string &SubscriberId,
						std::vector<std::string> &Devices);
	}

} // namespace OpenWifi::SDK::Prov