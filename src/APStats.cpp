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

            auto radios = (*State)["radios"];
            uint radio_index=0;
            std::map<uint,uint> radio_band;
            for(const auto &radio:radios) {
                if(radio.contains("channel")) {
                    radio_band[radio_index++] = radio["channel"] <= 16 ? 2 : 5;
                    // uint64_t active_ms = radio.contains("active_ms") ? radio["active_ms"].get<uint64_t>() : 0;
                    uint64_t active_ms, busy_ms, receive_ms, transmit_ms, tx_power, channel;
                    int64_t temperature, noise;

                    GetJSON("busy_ms",radio,busy_ms, (uint64_t)0 );
                    GetJSON("receive_ms",radio,receive_ms, (uint64_t)0);
                    GetJSON("transmit_ms",radio,transmit_ms, (uint64_t)0);
                    GetJSON("active_ms",radio,active_ms, (uint64_t)0);
                    GetJSON("tx_power",radio,tx_power, (uint64_t)0);
                    GetJSON("active_ms",radio,active_ms, (uint64_t)0);
                    GetJSON("channel",radio,channel, (uint64_t)0);
                    GetJSON("temperature",radio,temperature, (int64_t)0);
                    GetJSON("noise",radio,noise, (int64_t)0);
                }
            }

            //  now that we know the radio bands, look for associations
            auto interfaces = (*State)["interfaces"];
            DI_.associations_2g = DI_.associations_5g = DI_.associations_6g = 0;
            for(const auto &interface:interfaces) {
                if(interface.contains("counters")) {
                    auto counters = interface["counters"];

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
                                radio_location = std::atoi(radio_parts[2].c_str());
                            }
                        }
                        auto bssid = ssid.contains("bssid") ? ssid["bssid"] : "";
                        auto mode = ssid.contains("mode") ? ssid["mode"] : "";
                        auto ssid_name = ssid.contains("ssid") ? ssid["ssid"] : "";
                        if (ssid.contains("associations")) {
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
                                std::string association_bssid = association.contains("bssid") ? association["bssid"] : "";
                                std::string station = association.contains("station") ? association["station"] : "";
                                int64_t rssi = association.contains("rssi") ? association["rssi"].get<int64_t>() : 0;
                                uint64_t tx_bytes = association.contains("tx_bytes") ? association["tx_bytes"].get<uint64_t>() : 0;
                                uint64_t rx_bytes = association.contains("rx_bytes") ?  association["rx_bytes"].get<uint64_t>() : 0;
                                uint64_t tx_duration = association.contains("tx_duration") ? association["tx_duration"].get<uint64_t>() : 0;
                                uint64_t rx_packets = association.contains("rx_packets") ? association["rx_packets"].get<uint64_t>() : 0;
                                uint64_t tx_packets = association.contains("tx_packets") ? association["tx_packets"].get<uint64_t>() : 0;
                                uint64_t tx_retries = association.contains("tx_retries") ? association["tx_retries"].get<uint64_t>() : 0;
                                uint64_t tx_failed = association.contains("tx_failed") ? association["tx_failed"].get<uint64_t>() : 0;
                                uint64_t connected = association.contains("connected") ? association["connected"].get<uint64_t>() : 0;
                                uint64_t inactive = association.contains("inactive") ? association["inactive"].get<uint64_t>() : 0;
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
                GetJSON("timestamp", ping, DI_.lastConnection, 0ULL );
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
                DI_.lastDisconnection = Disconnection["timestamp"];
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
                if (ConnectionData.contains("connectionIp")) {
                    DI_.connectionIp = ConnectionData["connectionIp"];
                }
                if (ConnectionData.contains("locale")) {
                    DI_.locale = ConnectionData["locale"];
                }
            }
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": connection failed parsing." ;
            std::cout << *Connection << std::endl;
        }
    }

    void AP::UpdateHealth(const std::shared_ptr<nlohmann::json> & Health) {
        try {
            DI_.health = (*Health)["sanity"];
            DI_.lastHealth = (*Health)["timestamp"];
            std::cout << Utils::IntToSerialNumber(mac_) << ": health " << DI_.health << std::endl;
        } catch(...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": health failed parsing." << std::endl;
        }
    }

}