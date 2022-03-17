//
// Created by stephane bourque on 2022-03-11.
//

#include "RESTAPI_board_list_handler.h"
#include "StorageService.h"
#include "RESTAPI/RESTAPI_analytics_db_helpers.h"

namespace OpenWifi {
    void RESTAPI_board_list_handler::DoGet() {
        auto forVenue = GetParameter("forVenue","");

        if(!forVenue.empty()) {
            std::vector<AnalyticsObjects::BoardInfo>    Boards;
            auto F = [&](const AnalyticsObjects::BoardInfo &B) -> bool {
                if(!B.venueList.empty()) {
                    for(const auto &venue:B.venueList) {
                        if(venue.id == forVenue) {
                           Boards.emplace_back(B);
                           break;
                        }
                    }
                }
                return true;
            };
            DB_.Iterate(F);
            return ReturnObject("boards",Boards);
        }

        return ListHandler<BoardsDB>("boards", DB_, *this);
    }

}