//
// Created by stephane bourque on 2022-01-10.
//

#pragma once

#include "RESTAPI_ProvObjects.h"

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
        };

    }

}