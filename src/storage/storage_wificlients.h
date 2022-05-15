//
// Created by stephane bourque on 2022-05-15.
//

#pragma once

#include "framework/orm.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"

namespace OpenWifi {
    typedef Poco::Tuple<
        uint64_t,       //     timestamp=OpenWifi::Now();
        std::string,    //     stationId;
        std::string,    //     bssId;
        std::string,    //     ssid;
        int64_t,        //     rssi=0;
        uint32_t,       //     rx_bitrate=0;
        uint32_t,       //     rx_chwidth=0;
        uint16_t,       //     rx_mcs=0;
        uint16_t,       //     rx_nss=0;
        bool,           //     rx_vht=false;
        uint32_t,       //        tx_bitrate=0;
        uint32_t,       //        tx_chwidth=0;
        uint16_t,       //        tx_mcs=0;
        uint16_t,       //        tx_nss=0;
        bool,           //            tx_vht=false;
        uint64_t,       //        rx_bytes=0,
        uint64_t,       //      tx_bytes=0;
        int64_t,        //         rx_duration=0,
        uint64_t,       //  tx_duration=0;
        uint64_t,       //         rx_packets=0,
        uint64_t,       //  tx_packets=0;
        std::string,    //     ipv4;
        std::string,    //     ipv6;
        uint64_t,       //        channel_width=0;
        int64_t,        //         noise=0;
        uint64_t,       //        tx_power=0;
        uint64_t,       //        channel=0;
        uint64_t,       //        active_ms=0,
        uint64_t,       //      busy_ms=0,
        uint64_t,       //      receive_ms=0;
        std::string,    //     mode;
        int64_t,        //         ack_signal=0;
        int64_t,        //         ack_signal_avg=0;
        int64_t,        //         connected=0;
        int64_t,        //         inactive=0;
        int64_t         //         tx_retries=0;
    > WifiClientHistoryDBRecordType;

    class WifiClientHistoryDB : public ORM::DB<WifiClientHistoryDBRecordType, AnalyticsObjects::WifiClientHistory> {
    public:
        WifiClientHistoryDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L);
        virtual ~WifiClientHistoryDB() {};
        bool GetClientMacs(std::vector<std::string> &Macs);
    private:
        bool Upgrade(uint32_t from, uint32_t &to) override;
    };
}
