//
// Created by stephane bourque on 2022-05-15.
//

#include "RESTAPI_wificlienthistory_handler.h"
#include "WifiClientCache.h"

namespace OpenWifi {

    void RESTAPI_wificlienthistory_handler::DoGet() {

        auto venue = GetParameter("venue","");
        if(venue.empty()) {
            return BadRequest(RESTAPI::Errors::VenueMustExist);
        }

        if(GetBoolParameter("macsOnly")) {

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

        auto stationId = GetBinding("client");
        if(!Utils::ValidSerialNumber(stationId)) {
            return BadRequest(RESTAPI::Errors::InvalidSerialNumber);
        }

        auto orderBy = GetParameter("orderBy");
        if(orderBy.empty()) {
            orderBy = " order by timestamp DESC";
        } else {
            auto tokens = Poco::StringTokenizer(orderBy,":",Poco::StringTokenizer::TOK_TRIM);
            if(tokens.count()!=2 || (tokens[1]!="a" && tokens[1]!="d")) {
                return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
            }
            if(!StorageService()->WifiClientHistoryDB().ValidFieldName(tokens[0])) {
                return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
            }
            orderBy = fmt::format(" order by {} {}",tokens[0] , tokens[1]=="a" ? "asc" : "desc");
        }


        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);

        WifiClientHistoryDB::RecordVec Results;
        std::string Where;
        if(fromDate!=0 && endDate!=0)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp>={} and timestamp<={} ", venue, stationId, fromDate, endDate);
        else if(fromDate!=0 && endDate==0)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp>={} ", venue, stationId, fromDate);
        else if(fromDate==0 && endDate!=0)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp<={} ", venue, stationId, endDate);
        else
            Where = fmt::format(" venue_id='{}' and station_id='{}' ", venue, stationId);

        if(GetBoolParameter("countOnly")) {
            auto Count = DB_.Count(Where);
            return ReturnCountOnly(Count);
        }

        if(StorageService()->WifiClientHistoryDB().GetRecords(QB_.Offset,QB_.Limit, Results, Where, orderBy)) {
            return ReturnObject("entries",Results);
        }

        return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
    }

    void RESTAPI_wificlienthistory_handler::DoDelete() {

        if(UserInfo_.userinfo.userRole!=SecurityObjects::ADMIN && UserInfo_.userinfo.userRole!=SecurityObjects::ROOT) {
            return UnAuthorized(RESTAPI::Errors::ACCESS_DENIED);
        }

        auto venue = GetParameter("venue","");
        if(venue.empty()) {
            return BadRequest(RESTAPI::Errors::VenueMustExist);
        }

        auto stationId = GetBinding("client");
        if(!Utils::ValidSerialNumber(stationId)) {
            return BadRequest(RESTAPI::Errors::InvalidSerialNumber);
        }

        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);

        WifiClientHistoryDB::RecordVec Results;
        std::string Where;
        if(fromDate && endDate)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp>={} and timestamp<={} ", venue, stationId, fromDate, endDate);
        else if(fromDate && !endDate)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp>={} ", venue, stationId, fromDate);
        else if(!fromDate && endDate)
            Where = fmt::format(" venue_id='{}' and station_id='{}' and timestamp<={} ", venue, stationId, endDate);
        else
            Where = fmt::format("venue_id='{}' and  station_id='{}' ", stationId);

        if(StorageService()->WifiClientHistoryDB().DeleteRecords(Where)) {
            return OK();
        }

        return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
    }

}