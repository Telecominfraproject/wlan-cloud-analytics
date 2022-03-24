//
// Created by stephane bourque on 2022-03-21.
//

#pragma once

#include "framework/orm.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"

namespace OpenWifi {
    typedef Poco::Tuple<
            std::string,
            std::string,
            uint64_t,
            std::string,
            std::string,
            std::string,
            std::string
        > TimePointDBRecordType;

    class TimePointDB : public ORM::DB<TimePointDBRecordType, AnalyticsObjects::DeviceTimePoint> {
    public:
        TimePointDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L);
        bool GetStats(const std::string &id, AnalyticsObjects::DeviceTimePointStats &S);
        bool SelectRecords(const std::string &boardId, uint64_t FromDate, uint64_t LastDate, uint64_t MaxRecords, DB::RecordVec & Recs);
    private:
        bool Upgrade(uint32_t from, uint32_t &to) override;
    };
}
