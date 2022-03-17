//
// Created by stephane bourque on 2022-01-10.
//

#include "RESTAPI_AnalyticsObjects.h"
#include "RESTAPI_ProvObjects.h"
#include "framework/MicroService.h"

using OpenWifi::RESTAPI_utils::field_to_json;
using OpenWifi::RESTAPI_utils::field_from_json;

namespace OpenWifi::AnalyticsObjects {

    void Report::reset() {
    }

    void Report::to_json(Poco::JSON::Object &Obj) const {
    }

    void VenueInfo::to_json(Poco::JSON::Object &Obj) const {
        field_to_json(Obj,"id",id);
        field_to_json(Obj,"name",name);
        field_to_json(Obj,"description",description);
        field_to_json(Obj,"retention",retention);
        field_to_json(Obj,"interval",interval);
        field_to_json(Obj,"monitorSubVenues",monitorSubVenues);
    }

    bool VenueInfo::from_json(const Poco::JSON::Object::Ptr &Obj) {
        try {
            field_from_json(Obj,"id",id);
            field_from_json(Obj,"name",name);
            field_from_json(Obj,"description",description);
            field_from_json(Obj,"retention",retention);
            field_from_json(Obj,"interval",interval);
            field_from_json(Obj,"monitorSubVenues",monitorSubVenues);
            return true;
        } catch(...) {

        }
        return false;
    }

    void BoardInfo::to_json(Poco::JSON::Object &Obj) const {
        info.to_json(Obj);
        field_to_json(Obj,"venueList",venueList);
    }

    bool BoardInfo::from_json(const Poco::JSON::Object::Ptr &Obj) {
        try {
            info.from_json(Obj);
            field_from_json(Obj,"venueList",venueList);
            return true;
        } catch(...) {

        }
        return false;
    }

    void DeviceInfo::to_json(Poco::JSON::Object &Obj) const {
        field_to_json(Obj,"boardId",boardId);
        field_to_json(Obj,"type",type);
        field_to_json(Obj,"serialNumber",serialNumber);
        field_to_json(Obj,"deviceType",deviceType);
        field_to_json(Obj,"lastContact",lastContact);
        field_to_json(Obj,"lastPing",lastPing);
        field_to_json(Obj,"lastState",lastState);
        field_to_json(Obj,"lastFirmware",lastFirmware);
        field_to_json(Obj,"lastFirmwareUpdate",lastFirmwareUpdate);
        field_to_json(Obj,"lastConnection",lastConnection);
        field_to_json(Obj,"lastDisconnection",lastDisconnection);
        field_to_json(Obj,"pings",pings);
        field_to_json(Obj,"states",states);
        field_to_json(Obj,"connected",connected);
        field_to_json(Obj,"connectionIp",connectionIp);
        field_to_json(Obj,"associations_2g",associations_2g);
        field_to_json(Obj,"associations_5g",associations_5g);
        field_to_json(Obj,"associations_6g",associations_6g);
        field_to_json(Obj,"health",health);
        field_to_json(Obj,"lastHealth",lastHealth);
        field_to_json(Obj,"locale",locale);
        field_to_json(Obj,"uptime",uptime);
        field_to_json(Obj,"memory",memory);
    }

    bool DeviceInfo::from_json(const Poco::JSON::Object::Ptr &Obj) {
        try {
            field_from_json(Obj,"boardId",boardId);
            field_from_json(Obj,"type",type);
            field_from_json(Obj,"serialNumber",serialNumber);
            field_from_json(Obj,"deviceType",deviceType);
            field_from_json(Obj,"lastContact",lastContact);
            field_from_json(Obj,"lastPing",lastPing);
            field_from_json(Obj,"lastState",lastState);
            field_from_json(Obj,"lastFirmware",lastFirmware);
            field_from_json(Obj,"lastFirmwareUpdate",lastFirmwareUpdate);
            field_from_json(Obj,"lastConnection",lastConnection);
            field_from_json(Obj,"lastDisconnection",lastDisconnection);
            field_from_json(Obj,"pings",pings);
            field_from_json(Obj,"states",states);
            field_from_json(Obj,"connected",connected);
            field_from_json(Obj,"connectionIp",connectionIp);
            field_from_json(Obj,"associations_2g",associations_2g);
            field_from_json(Obj,"associations_5g",associations_5g);
            field_from_json(Obj,"associations_6g",associations_6g);
            field_from_json(Obj,"health",health);
            field_from_json(Obj,"lastHealth",lastHealth);
            field_from_json(Obj,"locale",locale);
            field_from_json(Obj,"uptime",uptime);
            field_from_json(Obj,"memory",memory);
            return true;
        } catch(...) {

        }
        return false;
    }

    void DeviceInfoList::to_json(Poco::JSON::Object &Obj) const {
        field_to_json(Obj,"devices",devices);
    }

    bool DeviceInfoList::from_json(const Poco::JSON::Object::Ptr &Obj) {
        try {
            field_from_json(Obj,"devices",devices);
            return true;
        } catch(...) {

        }
        return false;
    }

}