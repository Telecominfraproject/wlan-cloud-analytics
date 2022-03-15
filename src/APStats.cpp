//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

namespace OpenWifi {

    void AP::UpdateStats(const std::shared_ptr<nlohmann::json> &State) {
        DI_.states++;
        DI_.connected =true;
        DI_.lastPing = DI_.lastState = OpenWifi::Now();
        std::cout << "Stats update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
        std::cout << *State << std::endl;
    }

/*

 {"disconnection":{"serialNumber":"24f5a207a130","timestamp":1647277663}}

 {"capabilities":{"compatible":"linksys_ea8300","model":"Linksys EA8300 (Dallas)","network":{"lan":["eth0"],"wan":["eth1"]},"platform":"ap","switch":{"switch0":{"enable":true,"ports":[{"device":"eth0","need_tag":false,"num":0,"want_untag":true},{"num":1,"role":"lan"},{"num":2,"role":"lan"},{"num":3,"role":"lan"},{"num":4,"role":"lan"}],"reset":true,"roles":[{"device":"eth0","ports":"1 2 3 4 0","role":"lan"}]}},"wifi":{"platform/soc/a000000.wifi":{"band":["2G"],"channels":[1,2,3,4,5,6,7,8,9,10,11],"frequencies":[2412,2417,2422,2427,2432,2437,2442,2447,2452,2457,2462],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"platform/soc/a800000.wifi":{"band":["5G"],"channels":[36,40,44,48,52,56,60,64],"dfs_channels":[52,56,60,64],"frequencies":[5180,5200,5220,5240,5260,5280,5300,5320],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"soc/40000000.pci/pci0000:00/0000:00:00.0/0000:01:00.0":{"band":["5G"],"channels":[100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165],"dfs_channels":[100,104,108,112,116,120,124,128,132,136,140,144],"frequencies":[5500,5520,5540,5560,5580,5600,5620,5640,5660,5680,5700,5720,5745,5765,5785,5805,5825],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865696178}}},"connectionIp":"24f5a207a130@24.84.172.236:57052","firmware":"OpenWrt 21.02-SNAPSHOT r16399+116-c67509efd7 / TIP-devel-a0880ed","locale":"CA","serial":"24f5a207a130","uuid":1645814316}

 {"ping":{"compatible":"edgecore_eap101","connectionIp":"903cb3bb1ef4@[2604:3d08:9680:bd01:923c:b3ff:febb:1ef4]:48204","firmware":"OpenWrt 21.02-SNAPSHOT r16399+116-c67509efd7 / TIP-devel-a0880ed","serialNumber":"903cb3bb1ef4"}}

 *
 */

    void AP::UpdateConnection(const std::shared_ptr<nlohmann::json> &Connection) {
        DI_.pings++;
        DI_.lastContact = OpenWifi::Now();
        std::cout << "Connection update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
        std::cout << *Connection << std::endl;
        if (Connection->contains("ping")) {
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
            if(ping.contains("timestamp")) {
                DI_.lastConnection = ping["timestamp"];
            }
        } else if (Connection->contains("disconnection")) {
            auto Disconnection = (*Connection)["disconnection"];
            DI_.lastDisconnection = Disconnection["timestamp"];
            DI_.connected = false;
        } else if (Connection->contains("capabilities")) {
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
        }
    }
}