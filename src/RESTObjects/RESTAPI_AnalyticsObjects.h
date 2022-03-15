//
// Created by stephane bourque on 2022-01-10.
//

#pragma once

#include "RESTAPI_ProvObjects.h"
#include <vector>

namespace OpenWifi {

    namespace AnalyticsObjects {

        struct Report {
            uint64_t            snapShot=0;

            void reset();
            void to_json(Poco::JSON::Object &Obj) const;
        };

        struct VenueInfo {
            OpenWifi::Types::UUID_t     id;
            std::string                 name;
            std::string                 description;
            uint64_t                    retention=0;
            uint64_t                    interval=0;
            bool                        monitorSubVenues=false;

            void to_json(Poco::JSON::Object &Obj) const;
            bool from_json(const Poco::JSON::Object::Ptr &Obj);
        };

        struct BoardInfo {
            ProvObjects::ObjectInfo     info;
            std::vector<VenueInfo>      venueList;

            void to_json(Poco::JSON::Object &Obj) const;
            bool from_json(const Poco::JSON::Object::Ptr &Obj);

            inline bool operator<(const BoardInfo &bb) const {
                return info.id < bb.info.id;
            }

            inline bool operator==(const BoardInfo &bb) const {
                return info.id == bb.info.id;
            }
        };

        struct DeviceInfo {
            std::string     boardId;
            std::string     type;
            std::string     serialNumber;
            std::string     deviceType;
            uint64_t        lastContact;
            uint64_t        lastPing;
            uint64_t        lastState;
            std::string     lastFirmware;
            uint64_t        lastFirmwareUpdate;
            uint64_t        lastConnection;
            uint64_t        lastDisconnection;
            uint64_t        pings;
            uint64_t        states;
            bool            connected;
            std::string     connectionIp;
            uint64_t        associations_2g;
            uint64_t        associations_5g;
            uint64_t        associations_6g;
            uint64_t        health;
            uint64_t        lastHealth;

            void to_json(Poco::JSON::Object &Obj) const;
            bool from_json(const Poco::JSON::Object::Ptr &Obj);
        };

        struct DeviceInfoList {
            std::vector<DeviceInfo>     devices;

            void to_json(Poco::JSON::Object &Obj) const;
            bool from_json(const Poco::JSON::Object::Ptr &Obj);
        };
    }

}