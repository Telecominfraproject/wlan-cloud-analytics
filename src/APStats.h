//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include <mutex>
#include "framework/MicroService.h"
#include "nlohmann/json.hpp"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"

namespace OpenWifi {

    class AP {
    public:
        explicit AP(uint64_t mac, const std::string &BoardId, Poco::Logger &L) :
            boardId_(BoardId),
            Logger_(L)
        {
            DI_.serialNumber = Utils::IntToSerialNumber(mac);
        }

        void UpdateStats(const std::shared_ptr<nlohmann::json> & State);
        void UpdateConnection(const std::shared_ptr<nlohmann::json> & Connection);
        void UpdateHealth(const std::shared_ptr<nlohmann::json> & Health);

        [[nodiscard]] const AnalyticsObjects::DeviceInfo & Info() const { return DI_; }
    private:
        std::string                                     boardId_;
        AnalyticsObjects::DeviceInfo                    DI_;
        AnalyticsObjects::DeviceTimePoint               tp_base_;
        bool                                            got_health = false,
                                                        got_connection = false,
                                                        got_base = false;
        Poco::Logger                                    &Logger_;
        inline Poco::Logger & Logger() { return Logger_; }
    };
}
