//
// Created by stephane bourque on 2022-03-21.
//

#include "RESTAPI_board_timepoint_handler.h"
#include "StorageService.h"

namespace OpenWifi {
    void RESTAPI_board_timepoint_handler::DoGet() {

        auto id = GetBinding("id","");

        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);
        auto maxRecords = GetParameter("maxRecords",100);
        auto stats = GetBoolParameter("stats");

        if(stats) {
            AnalyticsObjects::DeviceTimePointStats  DTPS;
            Poco::JSON::Object  Answer;
            DB_.GetStats(id,DTPS);
            DTPS.to_json(Answer);
            return ReturnObject(Answer);
        }

        AnalyticsObjects::DeviceTimePointList   Points;
        StorageService()->TimePointsDB().SelectRecords(fromDate, endDate, maxRecords, Points.points);
        Poco::JSON::Object  Answer;
        Points.to_json(Answer);
        return ReturnObject(Answer);
    }
}