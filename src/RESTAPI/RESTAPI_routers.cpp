//
// Created by stephane bourque on 2021-10-23.
//

#include "RESTAPI/RESTAPI_board_devices_handler.h"
#include "RESTAPI/RESTAPI_board_handler.h"
#include "RESTAPI/RESTAPI_board_list_handler.h"
#include "RESTAPI/RESTAPI_board_timepoint_handler.h"
#include "RESTAPI/RESTAPI_wificlienthistory_handler.h"

#include "framework/RESTAPI_SystemCommand.h"
#include "framework/RESTAPI_WebSocketServer.h"
#include "framework/RESTAPI_SystemConfiguration.h"

namespace OpenWifi {

	Poco::Net::HTTPRequestHandler *
	RESTAPI_ExtRouter(const std::string &Path, RESTAPIHandler::BindingMap &Bindings,
					  Poco::Logger &L, RESTAPI_GenericServerAccounting &S, uint64_t TransactionId) {
		return RESTAPI_Router<RESTAPI_system_command, RESTAPI_system_configuration, RESTAPI_board_devices_handler,
							  RESTAPI_board_timepoint_handler, RESTAPI_board_handler,
							  RESTAPI_board_list_handler, RESTAPI_wificlienthistory_handler,
							  RESTAPI_webSocketServer>(Path, Bindings, L, S, TransactionId);
	}

	Poco::Net::HTTPRequestHandler *
	RESTAPI_IntRouter(const std::string &Path, RESTAPIHandler::BindingMap &Bindings,
					  Poco::Logger &L, RESTAPI_GenericServerAccounting &S, uint64_t TransactionId) {
		return RESTAPI_Router_I<RESTAPI_system_command, RESTAPI_system_configuration, RESTAPI_board_devices_handler,
								RESTAPI_board_timepoint_handler, RESTAPI_board_handler,
								RESTAPI_board_list_handler, RESTAPI_wificlienthistory_handler>(
			Path, Bindings, L, S, TransactionId);
	}

} // namespace OpenWifi