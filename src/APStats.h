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

    struct msdu_entry {
        uint64_t    rx_msdu=0,
                    tx_msdu=0,
                    tx_msdu_failed=0,
                    tx_msdu_retries=0;
    };

    struct UETimePoint {
        uint64_t    association_bssid=0,station=0;
        int64_t     rssi=0;
        uint64_t    tx_bytes=0,
                    rx_bytes=0,
                    tx_duration=0,
                    rx_packets=0,
                    tx_packets=0,
                    tx_retries=0,
                    tx_failed=0,
                    connected=0,
                    inactive=0;
        std::vector<msdu_entry>     msdus;
    };

    enum SSID_MODES {
        unknown=0,
        ap,
        mesh,
        sta,
        wds_ap,
        wds_sta,
        wds_repeater
    };

    inline SSID_MODES SSID_Mode(const std::string &m) {
        if(m=="ap")
            return ap;
        if(m=="sta")
            return sta;
        if(m=="mesh")
            return mesh;
        if(m=="wds-ap")
            return wds_ap;
        if(m=="wds-sta")
            return wds_sta;
        if(m=="wds-repeater")
            return wds_repeater;
        return unknown;
    }

    struct SSIDTimePoint {
        uint64_t                    bssid=0,
                                    mode=0,
                                    ssid=0;
        std::vector<UETimePoint>    associations;
    };


    struct APTimePoint {
        uint64_t    collisions=0,
                    multicast=0,
                    rx_bytes=0,
                    rx_dropped=0,
                    rx_errors=0,
                    rx_packets=0,
                    tx_bytes=0,
                    tx_dropped=0,
                    tx_errors=0,
                    tx_packets=0;
    };

    struct RadioTimePoint {
        uint        band=0,
                    radio_channel=0;
        uint64_t    active_ms=0,
                    busy_ms=0,
                    receive_ms=0,
                    transmit_ms=0,
                    tx_power=0,
                    channel=0;
        int64_t     temperature=0,
                    noise=0;
    };


    struct DeviceTimePoint {
        uint64_t                        timestamp=0;
        APTimePoint                     ap_data;
        std::vector<SSIDTimePoint>      ssid_data;
        std::vector<RadioTimePoint>     radio_data;
        AnalyticsObjects::DeviceInfo    device_info;
    };

    class AP {
    public:
        explicit AP(uint64_t mac) : mac_(mac) {
            DI_.serialNumber = Utils::IntToSerialNumber(mac);
        }

        void UpdateStats(const std::shared_ptr<nlohmann::json> & State);
        void UpdateConnection(const std::shared_ptr<nlohmann::json> & Connection);
        void UpdateHealth(const std::shared_ptr<nlohmann::json> & Health);

        const AnalyticsObjects::DeviceInfo & Info() const { return DI_; }
    private:
        uint64_t                        mac_=0;
        AnalyticsObjects::DeviceInfo    DI_;
        std::vector<DeviceTimePoint>    DTP_;
    };
}
