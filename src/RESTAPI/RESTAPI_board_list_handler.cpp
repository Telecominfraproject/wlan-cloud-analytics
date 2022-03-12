//
// Created by stephane bourque on 2022-03-11.
//

#include "RESTAPI_board_list_handler.h"
#include "StorageService.h"
#include "RESTAPI/RESTAPI_analytics_db_helpers.h"

namespace OpenWifi {
    void RESTAPI_board_list_handler::DoGet() {
        return ListHandler<BoardsDB>("boards", DB_, *this);
    }

}