//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"
#include "dict_ssid.h"
#include "StorageService.h"

namespace OpenWifi {

    template <typename T> void GetJSON(const char *field, const nlohmann::json & doc, T & v , const T & def ) {
        if(doc.contains(field) && !doc[field].is_null()) {
            v = doc[field].get<T>();
        } else {
            v = def;
        }
    }

    void AP::UpdateStats(const std::shared_ptr<nlohmann::json> &State) {
        DI_.states++;
        DI_.connected =true;

        AnalyticsObjects::DeviceTimePoint DTP;

        // find radios first to get associations.
        try {
            if(State->contains("unit")) {
                auto unit = (*State)["unit"];
                GetJSON("localtime", unit, DI_.lastState, (uint64_t) 0);
                GetJSON("uptime", unit, DI_.uptime, (uint64_t) 0);
                if(unit.contains("memory")) {
                    auto memory = unit["memory"];
                    uint64_t free_mem, total_mem;
                    GetJSON("free", memory, free_mem, (uint64_t) 0);
                    GetJSON("total", memory, total_mem, (uint64_t) 0);
                    if(total_mem) {
                        DI_.memory = ((double) (total_mem - free_mem) / (double)total_mem) * 100.0;
                    } else {
                        DI_.memory = 0.0;
                    }
                }
            }

            DTP.timestamp = DI_.lastState;

            std::map<uint, uint> radio_band;
            if(State->contains("radios") && (*State)["radios"].is_array()) {
                auto radios = (*State)["radios"];
                uint radio_index = 0;
                for (const auto &radio: radios) {
                    if (radio.contains("channel")) {
                        AnalyticsObjects::RadioTimePoint  RTP;
                        RTP.band = radio["channel"] <= 16 ? 2 : 5;
                        radio_band[radio_index++] = RTP.band;
                        GetJSON("busy_ms", radio, RTP.busy_ms, (uint64_t) 0);
                        GetJSON("receive_ms", radio, RTP.receive_ms, (uint64_t) 0);
                        GetJSON("transmit_ms", radio, RTP.transmit_ms, (uint64_t) 0);
                        GetJSON("active_ms", radio, RTP.active_ms, (uint64_t) 0);
                        GetJSON("tx_power", radio, RTP.tx_power, (uint64_t) 0);
                        GetJSON("active_ms", radio, RTP.active_ms, (uint64_t) 0);
                        GetJSON("channel", radio, RTP.channel, (uint64_t) 0);
                        GetJSON("temperature", radio, RTP.temperature, (int64_t) 0);
                        GetJSON("noise", radio, RTP.noise, (int64_t) 0);
                        DTP.radio_data.push_back(RTP);
                    }
                }
            }

            //  now that we know the radio bands, look for associations
            auto interfaces = (*State)["interfaces"];
            DI_.associations_2g = DI_.associations_5g = DI_.associations_6g = 0;
            for(const auto &interface:interfaces) {
                if(interface.contains("counters")) {
                    auto counters = interface["counters"];
                    GetJSON("collisions", counters, DTP.ap_data.collisions, (uint64_t) 0);
                    GetJSON("multicast", counters, DTP.ap_data.multicast, (uint64_t) 0);
                    GetJSON("rx_bytes", counters, DTP.ap_data.rx_bytes, (uint64_t) 0);
                    GetJSON("rx_dropped", counters, DTP.ap_data.rx_dropped, (uint64_t) 0);
                    GetJSON("rx_errors", counters, DTP.ap_data.rx_errors, (uint64_t) 0);
                    GetJSON("rx_packets", counters, DTP.ap_data.rx_packets, (uint64_t) 0);
                    GetJSON("tx_bytes", counters, DTP.ap_data.tx_bytes, (uint64_t) 0);
                    GetJSON("tx_dropped", counters, DTP.ap_data.tx_dropped, (uint64_t) 0);
                    GetJSON("tx_errors", counters, DTP.ap_data.tx_errors, (uint64_t) 0);
                    GetJSON("tx_packets", counters, DTP.ap_data.tx_packets, (uint64_t) 0);
                }

                if(interface.contains("ssids")) {
                    auto ssids = interface["ssids"];
                    for (const auto &ssid: ssids) {
                        AnalyticsObjects::SSIDTimePoint   SSIDTP;
                        uint radio_location=0;
                        SSIDTP.band = 2;
                        if(ssid.contains("radio")) {
                            auto radio = ssid["radio"];
                            if(radio.contains("$ref")) {
                                auto ref = radio["$ref"];
                                auto radio_parts = Poco::StringTokenizer(ref, "/");
                                if(radio_parts.count()==3) {
                                    radio_location = std::strtol(radio_parts[2].c_str(), nullptr, 10);
                                    if(radio_band.find(radio_location)!=radio_band.end()) {
                                        SSIDTP.band = radio_band[radio_location];
                                    }
                                }
                            }
                        }
                        GetJSON("bssid",ssid,SSIDTP.bssid, std::string{""});
                        GetJSON("mode",ssid,SSIDTP.mode, std::string{""} );
                        GetJSON("ssid",ssid,SSIDTP.ssid, std::string{""} );
                        if (ssid.contains("associations") && ssid["associations"].is_array()) {
                            auto associations = ssid["associations"];
                            auto it = radio_band.find(radio_location);
                            if(it!=radio_band.end()) {
                                auto the_radio = it->second;
                                if (the_radio == 2)
                                    DI_.associations_2g += associations.size();
                                else if (the_radio == 5)
                                    DI_.associations_5g += associations.size();
                                else if (the_radio == 6)
                                    DI_.associations_6g += associations.size();
                            }
                            for(const auto &association:associations) {
                                AnalyticsObjects::UETimePoint TP;
                                std::cout << "ASSOCIATIONDATA: " << association << std::endl;
                                GetJSON("station",association,TP.station, std::string{} );
                                GetJSON("rssi",association,TP.rssi, (int64_t)0 );
                                GetJSON("tx_bytes",association,TP.tx_bytes, (uint64_t)0 );
                                GetJSON("rx_bytes",association,TP.rx_bytes, (uint64_t)0 );
                                GetJSON("tx_duration",association,TP.tx_duration, (uint64_t)0 );
                                GetJSON("rx_packets",association,TP.rx_packets, (uint64_t)0 );
                                GetJSON("tx_packets",association,TP.tx_packets, (uint64_t)0 );
                                GetJSON("tx_retries",association,TP.tx_retries, (uint64_t)0 );
                                GetJSON("tx_failed",association,TP.tx_failed, (uint64_t)0 );
                                GetJSON("connected",association,TP.connected, (uint64_t)0 );
                                GetJSON("inactive",association,TP.inactive, (uint64_t)0 );

                                if(association.contains("msdu") && association["msdu"].is_array()) {
                                    auto msdus = association["msdu"];
                                    for(const auto &msdu:msdus) {
                                        AnalyticsObjects::MSDU_entry  E;
                                        GetJSON("rx_msdu",msdu,E.rx_msdu, (uint64_t)0 );
                                        GetJSON("tx_msdu",msdu,E.tx_msdu, (uint64_t)0 );
                                        GetJSON("tx_msdu_failed",msdu,E.tx_msdu_failed, (uint64_t)0 );
                                        GetJSON("tx_msdu_retries",msdu,E.tx_msdu_retries, (uint64_t)0 );
                                        TP.msdus.push_back(E);
                                    }
                                }

                                if(association.contains("tx_rate")) {
                                    auto tx_rate = association["tx_rate"];
                                    GetJSON("bitrate",tx_rate,TP.tx_rate.bitrate, (uint64_t)0 );
                                    GetJSON("mcs",tx_rate,TP.tx_rate.mcs, (uint64_t)0 );
                                    GetJSON("nss",tx_rate,TP.tx_rate.nss, (uint64_t)0 );
                                    GetJSON("chwidth",tx_rate,TP.tx_rate.chwidth, (uint64_t)0 );
                                    GetJSON("ht",tx_rate,TP.tx_rate.ht, false );
                                    GetJSON("sgi",tx_rate,TP.tx_rate.sgi, false );
                                }

                                if(association.contains("rx_rate")) {
                                    auto rx_rate = association["rx_rate"];
                                    GetJSON("bitrate",rx_rate,TP.rx_rate.bitrate, (uint64_t)0 );
                                    GetJSON("mcs",rx_rate,TP.rx_rate.mcs, (uint64_t)0 );
                                    GetJSON("nss",rx_rate,TP.rx_rate.nss, (uint64_t)0 );
                                    GetJSON("chwidth",rx_rate,TP.rx_rate.chwidth, (uint64_t)0 );
                                    GetJSON("ht",rx_rate,TP.rx_rate.ht, false );
                                    GetJSON("sgi",rx_rate,TP.rx_rate.sgi, false );
                                }

                                SSIDTP.associations.push_back(TP);
                            }
                        }
                        DTP.ssid_data.push_back(SSIDTP);
                    }
                }

            }
            DTP.device_info = DI_;
//            std::cout << Utils::IntToSerialNumber(mac_) << ": stats ";
//            std::cout << "2G: " << DI_.associations_2g << "   5G: " << DI_.associations_5g << "   6G: " << DI_.associations_6g << std::endl;
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": stats failed parsing." ;
            std::cout << *State << std::endl;
        }

        DTP.id = MicroService::instance().CreateUUID();
        DTP.boardId = boardId_;
        StorageService()->TimePointsDB().CreateRecord(DTP);

        DTP_.push_back(DTP);

        if(DTP_.size()>1000) {
            DTP_.erase(DTP_.begin());
        }

        std::cout << "Serial: " << Utils::IntToSerialNumber(mac_) << "  points: " << DTP_.size() << std::endl;
    }

    /*
{"ping":{"compatible":"edgecore_eap101","connectionIp":"903cb3b16e92@24.84.172.236:49620","firmware":"OpenWrt 21.02-SNAPSHOT r16399+116-c67509efd7 / TIP-devel-a0880ed","locale":"CA","serialNumber":"903cb3b16e92","timestamp":1647412525}}

     */


    void AP::UpdateConnection(const std::shared_ptr<nlohmann::json> &Connection) {
        DI_.pings++;
        DI_.lastContact = OpenWifi::Now();
        try {
            if (Connection->contains("ping")) {
                std::cout << Utils::IntToSerialNumber(mac_) << ": ping" << std::endl;
                DI_.connected = true;
                DI_.lastPing = OpenWifi::Now();
                auto ping = (*Connection)["ping"];
                GetJSON("compatible", ping, DI_.deviceType, std::string{} );
                GetJSON("connectionIp", ping, DI_.connectionIp, std::string{} );
                GetJSON("locale", ping, DI_.locale, std::string{} );
                GetJSON("timestamp", ping, DI_.lastConnection, (uint64_t)0 );
                if (ping.contains("firmware")) {
                    auto NewFirmware = ping["firmware"];
                    if (NewFirmware != DI_.lastFirmware) {
                        DI_.lastFirmware = NewFirmware;
                        DI_.lastFirmwareUpdate = OpenWifi::Now();
                    }
                }
            } else if (Connection->contains("disconnection")) {
                std::cout << Utils::IntToSerialNumber(mac_) << ": disconnection" << std::endl;
                auto Disconnection = (*Connection)["disconnection"];
                GetJSON("timestamp", Disconnection, DI_.lastDisconnection, (uint64_t)0 );
                DI_.connected = false;
            } else if (Connection->contains("capabilities")) {
                std::cout << Utils::IntToSerialNumber(mac_) << ": connection" << std::endl;
                DI_.connected = true;
                DI_.lastConnection = OpenWifi::Now();
                auto ConnectionData = (*Connection)["capabilities"];
                if (ConnectionData.contains("firmware")) {
                    auto NewFirmware = ConnectionData["firmware"];
                    if (NewFirmware != DI_.lastFirmware) {
                        DI_.lastFirmware = NewFirmware;
                        DI_.lastFirmwareUpdate = OpenWifi::Now();
                    }
                }
                GetJSON("connectionIp", ConnectionData, DI_.connectionIp, std::string{} );
                GetJSON("locale", ConnectionData, DI_.locale, std::string{} );
            }
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": connection failed parsing." ;
            std::cout << *Connection << std::endl;
        }
    }

    void AP::UpdateHealth(const std::shared_ptr<nlohmann::json> & Health) {
        try {
            GetJSON("timestamp", *Health, DI_.lastHealth, (uint64_t)0 );
            GetJSON("sanity", *Health, DI_.health, (uint64_t)0 );
            std::cout << Utils::IntToSerialNumber(mac_) << ": health " << DI_.health << std::endl;
        } catch(...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": health failed parsing." << std::endl;
        }
    }
}