//
// Created by stephane bourque on 2022-01-11.
//

#include "SDK_prov.h"
#include "framework/MicroServiceNames.h"
#include "framework/OpenAPIRequests.h"
#include "framework/OpenWifiTypes.h"

namespace OpenWifi::SDK::Prov {

	namespace Venue {
		bool GetDevices(RESTAPIHandler *client, const std::string &VenueId, bool WithChildren,
						ProvObjects::VenueDeviceList &DeviceList, bool &VenueExists) {
			std::string EndPoint = "/api/v1/venue/" + VenueId;

			VenueExists = true;
			auto Query = WithChildren ? OpenWifi::Types::StringPairVec{{"getDevices", "true"},
																	   {"getChildren", "true"}}
									  : OpenWifi::Types::StringPairVec{{"getDevices", "true"}};

			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint, Query, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();

			auto ResponseStatus = API.Do(
				CallResponse, client == nullptr ? "" : client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				try {
					return DeviceList.from_json(CallResponse);
				} catch (...) {
					return false;
				}
			} else if (ResponseStatus == Poco::Net::HTTPResponse::HTTP_NOT_FOUND) {
				VenueExists = false;
				return true;
			}
			return false;
		}

		bool Get(RESTAPIHandler *client, const std::string &VenueId, ProvObjects::Venue &Venue,
				 bool &Exists) {
			std::string EndPoint = "/api/v1/venue/" + VenueId;

			Exists = true;
			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint, {}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();

			auto ResponseStatus = API.Do(
				CallResponse, client == nullptr ? "" : client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				try {
					return Venue.from_json(CallResponse);
				} catch (...) {
					return false;
				}
			} else if (ResponseStatus == Poco::Net::HTTPResponse::HTTP_NOT_FOUND) {
				Exists = false;
			}
			return false;
		}

		bool Exists(RESTAPIHandler *client, const std::string &VenueId, bool &Exists) {
			std::string EndPoint = "/api/v1/venue/" + VenueId;

			Exists = false;
			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint, {}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();

			auto ResponseStatus = API.Do(
				CallResponse, client == nullptr ? "" : client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				Exists = true;
				return true;
			} else if (ResponseStatus == Poco::Net::HTTPResponse::HTTP_NOT_FOUND) {
				return true;
			}
			return false;
		}
	} // namespace Venue

	namespace Device {
		bool Get(RESTAPIHandler *client, const std::string &Mac,
				 ProvObjects::InventoryTag &Device) {
			std::string EndPoint = "/api/v1/inventory/" + Mac;

			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint, {}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();

			auto ResponseStatus = API.Do(CallResponse, client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				try {
					return Device.from_json(CallResponse);
				} catch (...) {
					return false;
				}
			}
			return false;
		}
	} // namespace Device

	namespace Configuration {
		bool Get(RESTAPIHandler *client, const std::string &ConfigUUID,
				 ProvObjects::DeviceConfiguration &Config) {
			std::string EndPoint = "/api/v1/configurations/" + ConfigUUID;
			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint, {}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();
			auto ResponseStatus = API.Do(CallResponse, client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				try {
					return Config.from_json(CallResponse);
				} catch (...) {
					return false;
				}
			}
			return false;
		}

		bool Delete(RESTAPIHandler *client, const std::string &ConfigUUID) {
			std::string EndPoint = "/api/v1/configurations/" + ConfigUUID;
			auto API = OpenAPIRequestDelete(uSERVICE_PROVISIONING, EndPoint, {}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();
			auto ResponseStatus = API.Do(client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				return true;
			}
			return false;
		}

		bool Create(RESTAPIHandler *client, const std::string &Mac,
					const ProvObjects::DeviceConfiguration &Config, std::string &ConfigUUID) {
			std::string EndPoint = "/api/v1/configurations/0";
			Poco::JSON::Object Body;
			Config.to_json(Body);
			auto API = OpenAPIRequestPost(uSERVICE_PROVISIONING, EndPoint, {}, Body, 10000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();
			auto ResponseStatus = API.Do(CallResponse, client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus != Poco::Net::HTTPResponse::HTTP_OK) {
				return false;
			}

			ProvObjects::DeviceConfiguration NewConfig;
			NewConfig.from_json(CallResponse);
			ConfigUUID = NewConfig.info.id;

			Body.clear();
			Body.set("serialNumber", Mac);
			Body.set("deviceConfiguration", NewConfig.info.id);
			EndPoint = "/api/v1/inventory/" + Mac;
			auto API2 = OpenAPIRequestPut(uSERVICE_PROVISIONING, EndPoint, {}, Body, 10000);
			CallResponse->clear();
			ResponseStatus = API2.Do(CallResponse, client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus != Poco::Net::HTTPResponse::HTTP_OK) {
				return false;
			}
			return true;
		}

		bool Update([[maybe_unused]] RESTAPIHandler *client,
					[[maybe_unused]] const std::string &ConfigUUID,
					[[maybe_unused]] ProvObjects::DeviceConfiguration &Config) {

			return false;
		}
	} // namespace Configuration

	namespace Subscriber {
		bool GetDevices(RESTAPIHandler *client, const std::string &SubscriberId,
						ProvObjects::InventoryTagList &Devices) {
			std::string EndPoint = "/api/v1/inventory";

			auto API = OpenAPIRequestGet(uSERVICE_PROVISIONING, EndPoint,
										 {{"subscriber", SubscriberId}}, 60000);
			auto CallResponse = Poco::makeShared<Poco::JSON::Object>();

			auto ResponseStatus = API.Do(CallResponse, client->UserInfo_.webtoken.access_token_);
			if (ResponseStatus == Poco::Net::HTTPServerResponse::HTTP_OK) {
				try {
					return Devices.from_json(CallResponse);
				} catch (...) {
					return false;
				}
			}
			return false;
		}
	} // namespace Subscriber

} // namespace OpenWifi::SDK::Prov