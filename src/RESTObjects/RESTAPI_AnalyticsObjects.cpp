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
}