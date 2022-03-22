//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include <mutex>
#include "framework/MicroService.h"
#include "nlohmann/json.hpp"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"

namespace OpenWifi {
    const uint32_t interval = 20;                    //  how big per sample
    const uint32_t ap_length = 2 * 24 * 60 * 60;        //  buffer length in seconds
    const uint32_t ue_length = 2 * 60 * 60;        //  buffer length in seconds
    const uint32_t ap_buffer_size = ap_length / interval;
    const uint32_t ue_buffer_size = ue_length / interval;

    class AP {
    public:
        explicit AP(uint64_t mac, const std::string &BoardId) :
            mac_(mac),
            boardId_(BoardId)
        {
            DI_.serialNumber = Utils::IntToSerialNumber(mac);
        }

        void UpdateStats(const std::shared_ptr<nlohmann::json> & State);
        void UpdateConnection(const std::shared_ptr<nlohmann::json> & Connection);
        void UpdateHealth(const std::shared_ptr<nlohmann::json> & Health);

        const AnalyticsObjects::DeviceInfo & Info() const { return DI_; }
    private:
        uint64_t                                        mac_=0;
        std::string                                     boardId_;
        AnalyticsObjects::DeviceInfo                    DI_;
        std::vector<AnalyticsObjects::DeviceTimePoint>  DTP_;
    };
}
