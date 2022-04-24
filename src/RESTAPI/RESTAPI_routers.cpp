//
// Created by stephane bourque on 2021-10-23.
//

#include "framework/MicroService.h"
#include "RESTAPI/RESTAPI_board_list_handler.h"
#include "RESTAPI/RESTAPI_board_handler.h"
#include "RESTAPI/RESTAPI_board_devices_handler.h"
#include "RESTAPI/RESTAPI_board_timepoint_handler.h"

namespace OpenWifi {

    Poco::Net::HTTPRequestHandler * RESTAPI_ExtRouter(const std::string &Path, RESTAPIHandler::BindingMap &Bindings,
                                                            Poco::Logger & L, RESTAPI_GenericServer & S,
                                                            uint64_t TransactionId) {
        return  RESTAPI_Router<
                    RESTAPI_system_command,
                    RESTAPI_board_devices_handler,
                    RESTAPI_board_timepoint_handler,
                    RESTAPI_board_handler,
                    RESTAPI_board_list_handler
                >(Path,Bindings,L, S, TransactionId);
    }

    Poco::Net::HTTPRequestHandler * RESTAPI_IntRouter(const std::string &Path, RESTAPIHandler::BindingMap &Bindings,
                                                            Poco::Logger & L, RESTAPI_GenericServer & S,
                                                            uint64_t TransactionId) {
        return RESTAPI_Router_I<
                RESTAPI_system_command,
                RESTAPI_board_devices_handler,
                RESTAPI_board_timepoint_handler,
                RESTAPI_board_handler,
                RESTAPI_board_list_handler
            >(Path, Bindings, L, S, TransactionId);
    }

}