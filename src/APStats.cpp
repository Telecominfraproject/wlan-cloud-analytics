//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

namespace OpenWifi {

/*

{"interfaces":[
 {  "clients":
        [{  "ipv4_addresses":["10.2.0.1"],
            "ipv6_addresses":["fe80:0:0:0:a8a2:caff:fe45:bc6b"],
            "mac":"e2:63:da:86:64:8e",
            "ports":["eth1"]}],
    "counters":
        {"collisions":0,"multicast":762813,"rx_bytes":181910484,"rx_dropped":0,"rx_errors":0,"rx_packets":1076144,"tx_bytes":2749349,"tx_dropped":0,"tx_errors":0,"tx_packets":12941},
    "dns_servers":["10.2.0.1"],
    "ipv4":{"addresses":["10.2.177.154/16"],
    "dhcp_server":"10.2.0.1","leasetime":86400},
    "location":"/interfaces/0",
    "name":"up0v0",
    "uptime":130177},
 {"counters":
    {"collisions":0,"multicast":0,"rx_bytes":0,"rx_dropped":0,"rx_errors":0,"rx_packets":0,"tx_bytes":1032226,"tx_dropped":0,"tx_errors":0,"tx_packets":4881},
    "ipv4":{"addresses":["192.168.1.1/24"]},
    "location":"/interfaces/1",
    "name":"down1v0",
    "ssids":[
        {   "bssid":"04:f8:f8:fc:3b:04",
            "iface":"wlan0",
            "location":"/interfaces/1/ssids/0",
            "mode":"ap",
            "phy":"soc/1b700000.pci/pci0001:00/0001:00:00.0/0001:01:00.0",
            "radio":{"$ref":"#/radios/0"},
            "ssid":"OpenWifi"},
        {   "bssid":"04:f8:f8:fc:3b:03",
            "iface":"wlan1",
            "location":"/interfaces/1/ssids/0",
            "mode":"ap",
            "phy":"soc/1b900000.pci/pci0002:00/0002:00:00.0/0002:01:00.0",
            "radio":{"$ref":"#/radios/1"},"ssid":"OpenWifi"}],
    "uptime":130177}],
    "link-state":{"downstream":{"eth1":{"carrier":0}},"upstream":{"eth0":{"carrier":1,"duplex":"full","speed":1000}}},"radios":[{"active_ms":130162935,"busy_ms":5320005,"channel":100,"channel_width":"80","noise":-103,"phy":"soc/1b700000.pci/pci0001:00/0001:00:00.0/0001:01:00.0","receive_ms":4466633,"transmit_ms":450846,"tx_power":24},{"active_ms":130169276,"busy_ms":21844515,"channel":6,"channel_width":"20","noise":-96,"phy":"soc/1b900000.pci/pci0002:00/0002:00:00.0/0002:01:00.0","receive_ms":18929929,"transmit_ms":421807,"tx_power":30}],"unit":{"load":[0.039063,0.02295,0.001465],"localtime":1647363036,"memory":{"buffered":6762496,"cached":17711104,"free":99299328,"total":217239552},"uptime":130207},"version":1}

 */


    void AP::UpdateStats(const std::shared_ptr<nlohmann::json> &State) {
        DI_.states++;
        DI_.connected =true;
        DI_.lastPing = DI_.lastState = OpenWifi::Now();
        std::cout << "Stats update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
//        std::cout << *State << std::endl;

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
            std::cout << "Stats update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
            std::cout << "2G:" << DI_.associations_2g << "  5G" << DI_.associations_5g << " 6G:" << DI_.associations_6g << std::endl;
        } catch (...) {
            std::cout << "Could not parse stats for " << Utils::IntToSerialNumber(mac_) << std::endl;
        }
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