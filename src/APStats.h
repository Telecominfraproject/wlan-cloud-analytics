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


    enum wifi_band {
        band_2g=0, band_5g=1, band_6g=2
    };

    struct AP_Point {
        uint32_t    rx_bytes=0, tx_bytes=0;
        uint32_t    rx_pkts=0,tx_pkts=0;
        u_char      bands[4];
    };

    struct UE_Point {
        uint32_t    bssid_index;
        int8_t      rssi;
        uint32_t    rx_bytes,tx_bytes;
        uint16_t    tx_retries;
        wifi_band   band;
    };

    class UE {
    public:
        UE(uint64_t Station):
            Station_(Station) {
        }

    private:
        uint64_t    last_contact_=0;
        uint64_t    Station_=0;
        uint64_t    Start_=std::time(nullptr);
        uint64_t    Current_BSSID_=0;
        uint32_t    Current_BSSID_index_=0;
        std::array<UE_Point,ue_buffer_size> Data_;
    };

    class AP {
    public:
        explicit AP(uint64_t mac) : mac_(mac) {

        }

        void UpdateStats(const std::shared_ptr<nlohmann::json> & State);
        void UpdateConnection(const std::shared_ptr<nlohmann::json> & Connection);
        const AnalyticsObjects::DeviceInfo & Info() const { return DI_; }
    private:
        uint64_t                        mac_=0;
        AnalyticsObjects::DeviceInfo    DI_;
        std::array<AP_Point,ap_buffer_size> Data_;
    };
}
