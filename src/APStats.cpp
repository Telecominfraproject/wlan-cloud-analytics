//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

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

        // find radios first to get associations.
        try {

            DI_.lastState = (*State)["unit"]["localtime"];

            std::map<uint, uint> radio_band;
            if(State->contains("radios") && (*State)["radios"].is_array()) {
                auto radios = (*State)["radios"];
                uint radio_index = 0;
                for (const auto &radio: radios) {
                    if (radio.contains("channel")) {
                        radio_band[radio_index++] = radio["channel"] <= 16 ? 2 : 5;
                        uint64_t active_ms, busy_ms, receive_ms, transmit_ms, tx_power, channel;
                        int64_t temperature, noise;

                        GetJSON("busy_ms", radio, busy_ms, (uint64_t) 0);
                        GetJSON("receive_ms", radio, receive_ms, (uint64_t) 0);
                        GetJSON("transmit_ms", radio, transmit_ms, (uint64_t) 0);
                        GetJSON("active_ms", radio, active_ms, (uint64_t) 0);
                        GetJSON("tx_power", radio, tx_power, (uint64_t) 0);
                        GetJSON("active_ms", radio, active_ms, (uint64_t) 0);
                        GetJSON("channel", radio, channel, (uint64_t) 0);
                        GetJSON("temperature", radio, temperature, (int64_t) 0);
                        GetJSON("noise", radio, noise, (int64_t) 0);
                    }
                }
            }

            //  now that we know the radio bands, look for associations
            auto interfaces = (*State)["interfaces"];
            DI_.associations_2g = DI_.associations_5g = DI_.associations_6g = 0;
            for(const auto &interface:interfaces) {
                if(interface.contains("counters")) {
                    auto counters = interface["counters"];
                    uint64_t collisions,multicast,rx_bytes,rx_dropped,rx_errors,rx_packets,tx_bytes,
                            tx_dropped,tx_errors,tx_packets;
                    GetJSON("collisions", counters, collisions, (uint64_t) 0);
                    GetJSON("multicast", counters, multicast, (uint64_t) 0);
                    GetJSON("rx_bytes", counters, rx_bytes, (uint64_t) 0);
                    GetJSON("rx_dropped", counters, rx_dropped, (uint64_t) 0);
                    GetJSON("rx_errors", counters, rx_errors, (uint64_t) 0);
                    GetJSON("rx_packets", counters, rx_packets, (uint64_t) 0);
                    GetJSON("tx_bytes", counters, tx_bytes, (uint64_t) 0);
                    GetJSON("tx_dropped", counters, tx_dropped, (uint64_t) 0);
                    GetJSON("tx_errors", counters, tx_errors, (uint64_t) 0);
                    GetJSON("tx_packets", counters, tx_packets, (uint64_t) 0);
                }

                if(interface.contains("ssids")) {
                    uint64_t uptime = interface.contains("uptime") ? interface["uptime"].get<uint64_t>() : 0;
                    auto ssids = interface["ssids"];
                    for (const auto &ssid: ssids) {
                        uint radio_location = 2;
                        if(ssid.contains("radio")) {
                            auto radio = ssid["radio"];
                            if(radio.contains("$ref")) {
                                auto ref = radio["$ref"];
                                auto radio_parts = Poco::StringTokenizer(ref, "/");
                                if(radio_parts.count()==3)
                                    radio_location = std::atoi(radio_parts[2].c_str());
                            }
                        }
                        std::string bssid, mode, ssid_name;
                        GetJSON("bssid",ssid,bssid, std::string{""} );
                        GetJSON("mode",ssid,mode, std::string{""} );
                        GetJSON("ssid",ssid,ssid_name, std::string{""} );
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
                                std::string association_bssid,station;
                                GetJSON("bssid",association,association_bssid, std::string{""} );
                                GetJSON("station",association,station, std::string{} );

                                int64_t rssi;
                                GetJSON("rssi",association,rssi, (int64_t)0 );

                                uint64_t tx_bytes, rx_bytes, tx_duration, rx_packets, tx_packets, tx_retries, tx_failed,
                                        connected,inactive;

                                GetJSON("tx_bytes",association,tx_bytes, (uint64_t)0 );
                                GetJSON("rx_bytes",association,rx_bytes, (uint64_t)0 );
                                GetJSON("tx_duration",association,tx_duration, (uint64_t)0 );
                                GetJSON("rx_packets",association,rx_packets, (uint64_t)0 );
                                GetJSON("tx_packets",association,tx_packets, (uint64_t)0 );
                                GetJSON("tx_retries",association,tx_retries, (uint64_t)0 );
                                GetJSON("tx_failed",association,tx_failed, (uint64_t)0 );
                                GetJSON("connected",association,connected, (uint64_t)0 );
                                GetJSON("inactive",association,inactive, (uint64_t)0 );
                            }
                        }
                    }
                }
            }
            std::cout << Utils::IntToSerialNumber(mac_) << ": stats ";
            std::cout << "2G: " << DI_.associations_2g << "   5G: " << DI_.associations_5g << "   6G: " << DI_.associations_6g << std::endl;
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": stats failed parsing." ;
            std::cout << *State << std::endl;
        }
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