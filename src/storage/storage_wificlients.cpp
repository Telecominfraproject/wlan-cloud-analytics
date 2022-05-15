//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "framework/MicroService.h"
#include "storage_wificlients.h"
#include "framework/OpenWifiTypes.h"

namespace OpenWifi {

    static  ORM::FieldVec    Boards_Fields{
            ORM::Field{"timestamp",ORM::FieldType::FT_BIGINT},
            ORM::Field{"stationId",ORM::FieldType::FT_TEXT},
            ORM::Field{"bssId",ORM::FieldType::FT_TEXT},
            ORM::Field{"ssid",ORM::FieldType::FT_TEXT},
            ORM::Field{"rssi",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_bitrate",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_chwidth",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_mcs",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_nss",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_vht",ORM::FieldType::FT_BOOLEAN},
            ORM::Field{"tx_bitrate",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_chwidth",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_mcs",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_nss",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_vht",ORM::FieldType::FT_BOOLEAN},
            ORM::Field{"rx_bytes",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_bytes",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_duration",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_duration",ORM::FieldType::FT_BIGINT},
            ORM::Field{"rx_packets",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_packets",ORM::FieldType::FT_BIGINT},
            ORM::Field{"ipv4",ORM::FieldType::FT_TEXT},
            ORM::Field{"ipv6",ORM::FieldType::FT_TEXT},
            ORM::Field{"channel_width",ORM::FieldType::FT_BIGINT},
            ORM::Field{"noise",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_power",ORM::FieldType::FT_BIGINT},
            ORM::Field{"channel",ORM::FieldType::FT_BIGINT},
            ORM::Field{"active_ms",ORM::FieldType::FT_BIGINT},
            ORM::Field{"busy_ms",ORM::FieldType::FT_BIGINT},
            ORM::Field{"receive_ms",ORM::FieldType::FT_BIGINT},
            ORM::Field{"mode",ORM::FieldType::FT_TEXT},
            ORM::Field{"ack_signal",ORM::FieldType::FT_BIGINT},
            ORM::Field{"ack_signal_avg",ORM::FieldType::FT_BIGINT},
            ORM::Field{"connected",ORM::FieldType::FT_BIGINT},
            ORM::Field{"inactive",ORM::FieldType::FT_BIGINT},
            ORM::Field{"tx_retries",ORM::FieldType::FT_BIGINT}
    };

    static  ORM::IndexVec    BoardsDB_Indexes{
            { std::string("stationid_name_index"),
              ORM::IndexEntryVec{
                      {std::string("stationId"),
                       ORM::Indextype::ASC} } },
            { std::string("stationtsid_name_index"),
              ORM::IndexEntryVec{
                      {std::string("stationId"),
                       ORM::Indextype::ASC} ,
                      {std::string("timestamp"),
                       ORM::Indextype::ASC}} }
    };

    WifiClientHistoryDB::WifiClientHistoryDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L) :
            DB(T, "wificlienthistory", Boards_Fields, BoardsDB_Indexes, P, L, "wfh") {}

    bool WifiClientHistoryDB::Upgrade([[maybe_unused]]  uint32_t from, uint32_t &to) {
        std::vector<std::string> Statements{
        };
        RunScript(Statements);
        to = 2;
        return true;
    }
}

template<> void ORM::DB<OpenWifi::WifiClientHistoryDBRecordType, OpenWifi::AnalyticsObjects::WifiClientHistory>::Convert(const OpenWifi::WifiClientHistoryDBRecordType &In, OpenWifi::AnalyticsObjects::WifiClientHistory &Out) {
    Out.timestamp = In.get<0>();
    Out.stationId = In.get<1>();
    Out.bssId= In.get<2>();
    Out.ssid= In.get<3>();
    Out.rssi= In.get<4>();
    Out.rx_bitrate= In.get<5>();
    Out.rx_chwidth= In.get<6>();
    Out.rx_mcs= In.get<7>();
    Out.rx_nss= In.get<8>();
    Out.rx_vht= In.get<9>();
    Out.tx_bitrate= In.get<10>();
    Out.tx_chwidth= In.get<11>();
    Out.tx_mcs= In.get<12>();
    Out.tx_nss= In.get<13>();
    Out.tx_vht= In.get<14>();
    Out.rx_bytes= In.get<15>();
    Out.tx_bytes= In.get<16>();
    Out.rx_duration= In.get<17>();
    Out.tx_duration= In.get<18>();
    Out.rx_packets= In.get<19>();
    Out.tx_packets= In.get<20>();
    Out.ipv4= In.get<21>();
    Out.ipv6= In.get<22>();
    Out.channel_width= In.get<23>();
    Out.noise= In.get<24>();
    Out.tx_power= In.get<25>();
    Out.channel= In.get<26>();
    Out.active_ms= In.get<27>();
    Out.busy_ms= In.get<28>();
    Out.receive_ms= In.get<29>();
    Out.mode= In.get<30>();
    Out.ack_signal= In.get<31>();
    Out.ack_signal_avg= In.get<32>();
    Out.connected= In.get<33>();
    Out.inactive= In.get<34>();
    Out.tx_retries= In.get<35>();
}

template<> void ORM::DB<OpenWifi::WifiClientHistoryDBRecordType, OpenWifi::AnalyticsObjects::WifiClientHistory>::Convert(const OpenWifi::AnalyticsObjects::WifiClientHistory &In, OpenWifi::WifiClientHistoryDBRecordType &Out) {
    Out.set<0>(In.timestamp);
    Out.set<1>(In.stationId);
    Out.set<2>(In.bssId);
    Out.set<3>(In.ssid);
    Out.set<4>(In.rssi);
    Out.set<5>(In.rx_bitrate);
    Out.set<6>(In.rx_chwidth);
    Out.set<7>(In.rx_mcs);
    Out.set<8>(In.rx_nss);
    Out.set<9>(In.rx_vht);
    Out.set<10>(In.tx_bitrate);
    Out.set<11>(In.tx_chwidth);
    Out.set<12>(In.tx_mcs);
    Out.set<13>(In.tx_nss);
    Out.set<14>(In.tx_vht);
    Out.set<15>(In.rx_bytes);
    Out.set<16>(In.tx_bytes);
    Out.set<17>(In.rx_duration);
    Out.set<18>(In.tx_duration);
    Out.set<19>(In.rx_packets);
    Out.set<20>(In.tx_packets);
    Out.set<21>(In.ipv4);
    Out.set<22>(In.ipv6);
    Out.set<23>(In.channel_width);
    Out.set<24>(In.noise);
    Out.set<25>(In.tx_power);
    Out.set<26>(In.channel);
    Out.set<27>(In.active_ms);
    Out.set<28>(In.busy_ms);
    Out.set<29>(In.receive_ms);
    Out.set<30>(In.mode);
    Out.set<31>(In.ack_signal);
    Out.set<32>(In.ack_signal_avg);
    Out.set<33>(In.connected);
    Out.set<34>(In.inactive);
    Out.set<35>(In.tx_retries);

}
