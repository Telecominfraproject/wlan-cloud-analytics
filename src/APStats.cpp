//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

namespace OpenWifi {

    void AP::UpdateStats(const std::shared_ptr<nlohmann::json> &State) {
        DI_.states++;
        DI_.connected =true;
        DI_.lastPing = DI_.lastState = OpenWifi::Now();

        // find radios first to get associations.
        try {
            auto radios = (*State)["radios"];
            uint radio_index=0;
            std::map<uint,uint> radio_band;
            for(const auto &radio:radios) {
                if(radio.contains("channel")) {
                    radio_band[radio_index++] = radio["channel"] <= 16 ? 2 : 5;
                }
            }

            //  now that we know the radio bands, look for associations
            auto interfaces = (*State)["interfaces"];
            DI_.associations_2g = DI_.associations_5g = DI_.associations_6g = 0;
            for(const auto &interface:interfaces) {
                if(interface.contains("ssids")) {
                    auto ssids = interface["ssids"];
                    for (const auto &ssid: ssids) {
                        auto radio = ssid["radio"]["$ref"];
                        auto radio_parts = Poco::StringTokenizer(radio, "/");
                        auto radio_location = std::atoi(radio_parts[2].c_str());
                        if (ssid.contains("associations")) {
                            auto associations = ssid["associations"];
                            auto the_radio = radio_band.find(radio_location)->second;
                            if(the_radio==2)
                                DI_.associations_2g += associations.size();
                            else if(the_radio==5)
                                DI_.associations_5g += associations.size();
                            else if(the_radio==6)
                                DI_.associations_6g += associations.size();
                            for(const auto &association:associations) {
                            }
                        }
                    }
                }
            }
            std::cout << Utils::IntToSerialNumber(mac_) << ": stats ";
            std::cout << "2G: " << DI_.associations_2g << "   5G: " << DI_.associations_5g << "   6G: " << DI_.associations_6g << std::endl;
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": stats failed parsing." << std::endl;
        }
    }

    void AP::UpdateConnection(const std::shared_ptr<nlohmann::json> &Connection) {
        DI_.pings++;
        DI_.lastContact = OpenWifi::Now();
        try {
            if (Connection->contains("ping")) {
                std::cout << Utils::IntToSerialNumber(mac_) << ": ping" << std::endl;
                DI_.connected = true;
                DI_.lastPing = OpenWifi::Now();
                auto ping = (*Connection)["ping"];
                if (ping.contains("compatible"))
                    DI_.deviceType = ping["compatible"];
                if (ping.contains("firmware")) {
                    auto NewFirmware = ping["firmware"];
                    if (NewFirmware != DI_.lastFirmware) {
                        DI_.lastFirmware = NewFirmware;
                        DI_.lastFirmwareUpdate = OpenWifi::Now();
                    }
                }
                if (ping.contains("connectionIp")) {
                    DI_.connectionIp = ping["connectionIp"];
                }
                if (ping.contains("timestamp")) {
                    DI_.lastConnection = ping["timestamp"];
                }
                if (ping.contains("locale")) {
                    DI_.lastConnection = ping["locale"];
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
                    DI_.lastConnection = ConnectionData["locale"];
                }
            }
        } catch (...) {
            std::cout << Utils::IntToSerialNumber(mac_) << ": connection failed parsing." << std::endl;
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