//
// Created by stephane bourque on 2022-03-21.
//

#include "RESTAPI_board_timepoint_handler.h"
#include "StorageService.h"

namespace OpenWifi {
    void RESTAPI_board_timepoint_handler::DoGet() {

        std::cout << __LINE__ << std::endl;
        auto id = GetBinding("id","");

        std::cout << __LINE__ << std::endl;
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        std::cout << __LINE__ << std::endl;
        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        std::cout << __LINE__ << std::endl;
        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);
        auto maxRecords = GetParameter("maxRecords",100);
        auto stats = GetBoolParameter("stats");

        std::cout << __LINE__ << std::endl;
        if(stats) {
            std::cout << __LINE__ << std::endl;
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
        std::cout << __LINE__ << std::endl;
        return ReturnObject(Answer);
    }
}