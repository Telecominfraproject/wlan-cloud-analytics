//
// Created by stephane bourque on 2022-03-21.
//

#pragma once

#include "framework/MicroService.h"
#include "StorageService.h"

namespace OpenWifi {

    class RESTAPI_board_timepoint_handler : public RESTAPIHandler {
    public:
        RESTAPI_board_timepoint_handler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L, RESTAPI_GenericServer & Server, uint64_t TransactionId, bool Internal)
                : RESTAPIHandler(bindings, L,
                                 std::vector<std::string>{
                                         Poco::Net::HTTPRequest::HTTP_GET,
                                         Poco::Net::HTTPRequest::HTTP_DELETE,
                                         Poco::Net::HTTPRequest::HTTP_OPTIONS},
                                 Server,
                                 TransactionId,
                                 Internal){}

        static const std::list<const char *> PathName() { return std::list<const char *>{"/api/v1/board/{id}/timepoints"}; };

    private:
        TimePointDB   & DB_=StorageService()->TimePointsDB();
        void DoGet() final;
        void DoPost() final {};
        void DoPut() final {};
        void DoDelete() final;
    };
}
