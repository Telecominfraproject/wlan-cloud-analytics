//
// Created by stephane bourque on 2022-05-15.
//

#include "RESTAPI_wificlienthistory_handler.h"
#include "WifiClientCache.h"

namespace OpenWifi {

    void RESTAPI_wificlienthistory_handler::DoGet() {

        std::cout << __LINE__ << std::endl;
        if(GetBoolParameter("macsOnly")) {
            auto venue = GetParameter("venue","");
            if(venue.empty()) {
                return BadRequest(RESTAPI::Errors::VenueMustExist);
            }

            auto macFilter = GetParameter("macFilter","");
            std::vector<uint64_t>    Macs;
            WifiClientCache()->FindNumbers(venue,macFilter,500,Macs);
            Poco::JSON::Array   Arr;
            for(const auto &mac: Macs)
                Arr.add(Utils::IntToSerialNumber(mac));
            Poco::JSON::Object  Answer;
            Answer.set("entries", Arr);
            return ReturnObject(Answer);
        }

        std::cout << __LINE__ << std::endl;
        auto stationId = GetBinding("client");
        std::cout << __LINE__ << std::endl;
        if(!Utils::ValidSerialNumber(stationId)) {
            return BadRequest(RESTAPI::Errors::InvalidSerialNumber);
        }

        std::cout << __LINE__ << std::endl;
        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);

        std::cout << __LINE__ << std::endl;
        WifiClientHistoryDB::RecordVec Results;
        std::string Where;
        std::cout << __LINE__ << std::endl;
        if(fromDate && endDate)
            Where = fmt::format(" stationId='{}' and timestamp>={} and timestamp<={} ", stationId, fromDate, endDate);
        else if(fromDate && !endDate)
            Where = fmt::format(" stationId='{}' and timestamp>={} ", stationId, fromDate);
        else if(!fromDate && endDate)
            Where = fmt::format(" stationId='{}' and timestamp<={} ", stationId, endDate);
        else
            Where = fmt::format(" stationId='{}' ", stationId);

        std::cout << __LINE__ << std::endl;
        if(StorageService()->WifiClientHistoryDB().GetRecords(QB_.Offset,QB_.Limit, Results, Where)) {
            return ReturnObject("entries",Results);
        }

        std::cout << __LINE__ << std::endl;
        return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
    }

    void RESTAPI_wificlienthistory_handler::DoDelete() {

        std::cout << __LINE__ << std::endl;
        if(UserInfo_.userinfo.userRole!=SecurityObjects::ADMIN && UserInfo_.userinfo.userRole!=SecurityObjects::ROOT) {
            return UnAuthorized(RESTAPI::Errors::ACCESS_DENIED);
        }

        std::cout << __LINE__ << std::endl;
        auto stationId = GetBinding("client");
        if(!Utils::ValidSerialNumber(stationId)) {
            return BadRequest(RESTAPI::Errors::InvalidSerialNumber);
        }

        std::cout << __LINE__ << std::endl;
        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);

        WifiClientHistoryDB::RecordVec Results;
        std::string Where;
        if(fromDate && endDate)
            Where = fmt::format(" stationId='{}' and timestamp>={} and timestamp<={} ", stationId, fromDate, endDate);
        else if(fromDate && !endDate)
            Where = fmt::format(" stationId='{}' and timestamp>={} ", stationId, fromDate);
        else if(!fromDate && endDate)
            Where = fmt::format(" stationId='{}' and timestamp<={} ", stationId, endDate);
        else
            Where = fmt::format(" stationId='{}' ", stationId);

        std::cout << __LINE__ << std::endl;
        if(StorageService()->WifiClientHistoryDB().DeleteRecords(Where)) {
            return OK();
        }

        std::cout << __LINE__ << std::endl;
        return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
    }

}