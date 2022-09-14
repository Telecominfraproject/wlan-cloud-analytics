//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"
#include "dict_ssid.h"
#include "StorageService.h"
#include "WifiClientCache.h"

namespace OpenWifi {

    static std::string mac_filter(const std::string &m) {
        std::string r;
        for(const auto &c:m)
            if(c!=':' && c!='-') r += c;
        return r;
    }

    template <typename T> void GetJSON(const char *field, const nlohmann::json & doc, T & v , const T & def ) {
        if(doc.contains(field) && !doc[field].is_null()) {
            v = doc[field].get<T>();
            return;
        }
        v = def;
    }

    template <typename T> void GetJSON(const char *field1, const char *field2, const nlohmann::json & doc, T & v , const T & def ) {
        if(doc.contains(field1) && !doc[field1].is_null()) {
            auto subDoc = doc[field1];
            if(subDoc.contains(field2) && !subDoc[field2].is_null()) {
                v = subDoc[field2].get<T>();
                return;
            }
        }
        v = def;
    }

    inline double safe_div(uint64_t a , uint64_t b) {
        if(b==0)
            return 0.0;
        return (double)a/ (double) b;
    }

    inline double safe_pct(uint64_t a , uint64_t b) {
        if(b==0)
            return 0.0;
        return (100.0) * (double) a/ (double) b;
    }

    inline bool find_ue(const std::string &station, const std::vector<AnalyticsObjects::SSIDTimePoint> &tps, AnalyticsObjects::UETimePoint &ue_tp) {
        for(const auto &ssid:tps) {
            for(const auto &association:ssid.associations) {
                if(association.station==station) {
                    ue_tp = association;
                    return true;
                }
            }
        }
        return false;
    }

    template <typename X, typename M> double Average( X T, const std::vector<M> &Values ) {
        double result = 0.0;

        if(!Values.empty()) {
            double sum = 0.0;

            for(const auto &v:Values) {
                sum += (v.*T);
            }

            result = sum / (double) Values.size();
        }

        return result;
    }

    //  This is used to detect a new association VS an existing one. This may happen when a device is connected, disconnects, and
    //  reconnects in between 2 samples.
    static bool new_connection(const AnalyticsObjects::UETimePoint &Existing_UE, const AnalyticsObjects::UETimePoint &new_association) {
        if( new_association.tx_packets < Existing_UE.tx_packets ||
            new_association.rx_packets < Existing_UE.rx_packets ||
            new_association.tx_bytes < Existing_UE.tx_bytes     ||
            new_association.rx_bytes < Existing_UE.rx_bytes     ||
            new_association.tx_failed < Existing_UE.tx_failed) {
            return true;
        }
        return false;
    }

    void AP::UpdateStats(const std::shared_ptr<nlohmann::json> &State) {
        DI_.states++;
        DI_.connected =true;

        AnalyticsObjects::DeviceTimePoint DTP;
        Logger().information(fmt::format("{}: stats message.", DI_.serialNumber));

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

            std::map<uint, std::pair<uint,uint> > radio_map;
            if(State->contains("radios") && (*State)["radios"].is_array()) {
                auto radios = (*State)["radios"];
                uint radio_index = 0;
                for (const auto &radio: radios) {
                    if (radio.contains("channel")) {
                        AnalyticsObjects::RadioTimePoint RTP;
                        GetJSON("channel", radio, RTP.channel, (uint64_t) 2);
                        RTP.band = RTP.channel <= 16 ? 2 : 5;
                        radio_map[radio_index++] = std::make_pair(RTP.band, RTP.channel);
                        GetJSON("busy_ms", radio, RTP.busy_ms, (uint64_t) 0);
                        GetJSON("receive_ms", radio, RTP.receive_ms, (uint64_t) 0);
                        GetJSON("transmit_ms", radio, RTP.transmit_ms, (uint64_t) 0);
                        GetJSON("active_ms", radio, RTP.active_ms, (uint64_t) 0);
                        GetJSON("tx_power", radio, RTP.tx_power, (uint64_t) 0);
                        GetJSON("active_ms", radio, RTP.active_ms, (uint64_t) 0);
                        GetJSON("channel", radio, RTP.channel, (uint64_t) 0);
                        GetJSON("temperature", radio, RTP.temperature, (int64_t) 20);
                        if (radio.contains("channel_width") && !radio["channel_width"].is_null()) {
                            if(radio["channel_width"].is_string()) {
                                std::string C = radio["channel_width"];
                                RTP.channel_width = std::strtoull(C.c_str(), nullptr,10);
                            } else if(radio["channel_width"].is_number_integer()) {
                                RTP.channel_width = radio["channel_width"];
                            } else {
                                RTP.channel_width = 20;
                            }
                        }
                        if(RTP.temperature==0)
                            RTP.temperature = 20;
                        GetJSON("noise", radio, RTP.noise, (int64_t) -90);
                        if(RTP.noise==0)
                            RTP.noise=-90;
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

                InterfaceClientEntryMap_t ICEM;
                if(interface.contains("clients") && interface["clients"].is_array()) {
                    try {
                        auto Clients = interface["clients"];
                        for(const auto & client: Clients) {
                            if(client.contains("mac") && client["mac"].is_string()) {
                                InterfaceClientEntry E;
                                if(client.contains("ipv4_addresses") && client["ipv4_addresses"].is_array()) {
                                    for(const auto &ip:client["ipv4_addresses"])
                                        E.ipv4_addresses.push_back(ip);
                                }
                                if(client.contains("ipv6_addresses") && client["ipv6_addresses"].is_array()) {
                                    for(const auto &ip:client["ipv6_addresses"])
                                        E.ipv6_addresses.push_back(ip);
                                }
                                ICEM[client["mac"]] = E;
                            }
                        }
                    } catch(...) {

                    }
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
                                    if(radio_map.find(radio_location)!=radio_map.end()) {
                                        SSIDTP.band = radio_map[radio_location].first;
                                        SSIDTP.channel = radio_map[radio_location].second;
                                    }
                                }
                            }
                        }
                        GetJSON("bssid",ssid,SSIDTP.bssid, std::string{""});
                        GetJSON("mode",ssid,SSIDTP.mode, std::string{""} );
                        GetJSON("ssid",ssid,SSIDTP.ssid, std::string{""} );
                        if (ssid.contains("associations") && ssid["associations"].is_array()) {
                            auto associations = ssid["associations"];
                            auto radio_it = radio_map.find(radio_location);
                            if(radio_it!=radio_map.end()) {
                                auto the_radio = radio_it->second.first;
                                if (the_radio == 2)
                                    DI_.associations_2g += associations.size();
                                else if (the_radio == 5)
                                    DI_.associations_5g += associations.size();
                                else if (the_radio == 6)
                                    DI_.associations_6g += associations.size();
                            }
                            for(const auto &association:associations) {
                                AnalyticsObjects::UETimePoint TP;
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

                                AnalyticsObjects::WifiClientHistory WFH;
                                WFH.station_id = mac_filter(TP.station);
                                WFH.bssid = mac_filter(SSIDTP.bssid);
                                WFH.ssid = SSIDTP.ssid;
                                WFH.rssi = TP.rssi;
                                GetJSON("rx_rate","bitrate",association,WFH.rx_bitrate,(uint32_t)0);
                                GetJSON("rx_rate","chwidth",association,WFH.rx_chwidth,(uint32_t)0);
                                GetJSON("rx_rate","mcs",association,WFH.rx_mcs,(uint16_t)0);
                                GetJSON("rx_rate","nss",association,WFH.rx_nss,(uint16_t)0);
                                GetJSON("rx_rate","vht",association,WFH.rx_vht,false);
                                GetJSON("tx_rate","bitrate",association,WFH.tx_bitrate,(uint32_t)0);
                                GetJSON("tx_rate","chwidth",association,WFH.tx_chwidth,(uint32_t)0);
                                GetJSON("tx_rate","mcs",association,WFH.tx_mcs,(uint16_t)0);
                                GetJSON("tx_rate","nss",association,WFH.tx_nss,(uint16_t)0);
                                GetJSON("tx_rate","vht",association,WFH.tx_vht,false);
                                GetJSON("rx_bytes",association,WFH.rx_bytes,(uint64_t)0);
                                GetJSON("tx_bytes",association,WFH.tx_bytes,(uint64_t)0);
                                GetJSON("rx_duration",association,WFH.rx_duration,(uint64_t)0);
                                GetJSON("tx_duration",association,WFH.tx_duration,(uint64_t)0);
                                GetJSON("rx_packets",association,WFH.rx_packets,(uint64_t)0);
                                GetJSON("tx_packets",association,WFH.tx_packets,(uint64_t)0);

                                // try to locate the IP addresses
                                auto ClientInfo = ICEM.find(WFH.station_id);
                                if(ClientInfo!=end(ICEM)) {
                                    if(!ClientInfo->second.ipv4_addresses.empty())
                                        WFH.ipv4 = ClientInfo->second.ipv4_addresses[0];
                                    if(!ClientInfo->second.ipv6_addresses.empty())
                                        WFH.ipv6 = ClientInfo->second.ipv6_addresses[0];
                                }

                                for(const auto &rd:DTP.radio_data) {
                                    if(rd.band == SSIDTP.band) {
                                        WFH.channel_width = rd.channel_width;
                                        WFH.noise = rd.noise;
                                        WFH.tx_power = rd.tx_power;
                                        WFH.channel = rd.channel;
                                        WFH.active_ms = rd.active_ms;
                                        WFH.busy_ms = rd.busy_ms;
                                        WFH.receive_ms = rd.receive_ms;
                                        break;
                                    }
                                }

                                WFH.mode = SSIDTP.mode;
                                GetJSON("ack_signal",association,WFH.ack_signal,(int64_t)0);
                                GetJSON("ack_signal_avg",association,WFH.ack_signal_avg,(int64_t)0);
                                GetJSON("connected",association,WFH.connected,(uint64_t)0);
                                GetJSON("inactive",association,WFH.inactive,(uint64_t)0);
                                GetJSON("tx_retries",association,WFH.tx_retries,(uint64_t)0);

                                WifiClientCache()->AddSerialNumber(venue_id_,WFH.station_id);
                                WFH.venue_id = venue_id_;
                                StorageService()->WifiClientHistoryDB().CreateRecord(WFH);

                                if(association.contains("tid_stats") && association["tid_stats"].is_array()) {
                                    auto tid_stats = association["tid_stats"];
                                    for(const auto &tid_stat:tid_stats) {
                                        AnalyticsObjects::TIDstat_entry  E;
                                        GetJSON("rx_msdu",tid_stat,E.rx_msdu, (uint64_t)0 );
                                        GetJSON("tx_msdu",tid_stat,E.tx_msdu, (uint64_t)0 );
                                        GetJSON("tx_msdu_failed",tid_stat,E.tx_msdu_failed, (uint64_t)0 );
                                        GetJSON("tx_msdu_retries",tid_stat,E.tx_msdu_retries, (uint64_t)0 );
                                        TP.tidstats.push_back(E);
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
        } catch (...) {
            Logger().information(fmt::format("{}: stats failed parsing.", DI_.serialNumber));
        }

        if(got_base) {
            // calculate new point based on base, save new point, move DTP into base...
            AnalyticsObjects::DeviceTimePoint db_DTP = DTP;

            auto time_lapse = DTP.timestamp - tp_base_.timestamp;
            if(time_lapse==0) time_lapse = 1;

            db_DTP.ap_data.tx_bytes_delta =  db_DTP.ap_data.tx_bytes - tp_base_.ap_data.tx_bytes;
            db_DTP.ap_data.rx_bytes_delta =  db_DTP.ap_data.rx_bytes - tp_base_.ap_data.rx_bytes;
            db_DTP.ap_data.tx_packets_delta = db_DTP.ap_data.tx_packets - tp_base_.ap_data.tx_packets;
            db_DTP.ap_data.rx_packets_delta = db_DTP.ap_data.rx_packets - tp_base_.ap_data.rx_packets;
            db_DTP.ap_data.tx_dropped_delta = db_DTP.ap_data.tx_dropped - tp_base_.ap_data.tx_dropped;
            db_DTP.ap_data.rx_dropped_delta = db_DTP.ap_data.rx_dropped - tp_base_.ap_data.rx_dropped;
            db_DTP.ap_data.tx_errors_delta = db_DTP.ap_data.tx_errors - tp_base_.ap_data.tx_errors;
            db_DTP.ap_data.rx_errors_delta = db_DTP.ap_data.rx_errors - tp_base_.ap_data.rx_errors;

            db_DTP.ap_data.tx_bytes_bw =  safe_div(db_DTP.ap_data.tx_bytes_delta, time_lapse);
            db_DTP.ap_data.rx_bytes_bw =  safe_div(db_DTP.ap_data.rx_bytes_delta , time_lapse);
            db_DTP.ap_data.tx_packets_bw = safe_div(db_DTP.ap_data.tx_packets_delta, time_lapse);
            db_DTP.ap_data.rx_packets_bw = safe_div(db_DTP.ap_data.rx_packets_delta, time_lapse);
            db_DTP.ap_data.tx_dropped_pct = safe_pct(db_DTP.ap_data.tx_dropped_delta, db_DTP.ap_data.tx_packets);
            db_DTP.ap_data.rx_dropped_pct = safe_pct(db_DTP.ap_data.rx_dropped_delta, db_DTP.ap_data.rx_packets);
            db_DTP.ap_data.tx_errors_pct = safe_pct(db_DTP.ap_data.tx_errors_delta, db_DTP.ap_data.tx_packets);
            db_DTP.ap_data.rx_errors_pct = safe_pct(db_DTP.ap_data.rx_errors_delta, db_DTP.ap_data.rx_packets);

            for(auto &radio:db_DTP.radio_data) {
                bool found=false;
                for(const auto &base_radio:tp_base_.radio_data) {

                    if(radio.channel==base_radio.channel) {
                        found = true;
                        radio.active_pct = safe_pct( (radio.active_ms - base_radio.active_ms) / 1000 , time_lapse);
                        radio.busy_pct = safe_pct( (radio.busy_ms - base_radio.busy_ms) / 1000 , time_lapse);
                        radio.receive_pct = safe_pct( (radio.receive_ms - base_radio.receive_ms) / 1000 , time_lapse);
                        radio.transmit_pct = safe_pct( (radio.transmit_ms - base_radio.transmit_ms) / 1000 ,time_lapse);
                    }
                }

                if(!found) {
                    radio.active_pct = safe_pct(radio.active_ms / 1000, time_lapse);
                    radio.busy_pct = safe_pct(radio.busy_ms / 1000, time_lapse);
                    radio.receive_pct = safe_pct(radio.receive_ms / 1000, time_lapse);
                    radio.transmit_pct = safe_pct(radio.transmit_ms / 1000, time_lapse);
                }
            }

            for(auto &ssid:db_DTP.ssid_data) {
                for(auto &association:ssid.associations) {
                    AnalyticsObjects::UETimePoint ue_tp;
                    if(find_ue(association.station, tp_base_.ssid_data, ue_tp) && !new_connection(ue_tp,association)) {
                        association.tx_bytes_delta = association.tx_bytes - ue_tp.tx_bytes;
                        association.rx_bytes_delta = association.rx_bytes - ue_tp.rx_bytes;
                        association.tx_packets_delta = association.tx_packets - ue_tp.tx_packets;
                        association.rx_packets_delta = association.rx_packets - ue_tp.rx_packets;
                        association.tx_failed_delta = association.tx_failed - ue_tp.tx_failed;
                        association.tx_retries_delta = association.tx_retries - ue_tp.tx_retries;
                        association.tx_duration_delta = association.tx_duration - ue_tp.tx_duration;
                    } else {
                        association.tx_bytes_delta = association.tx_bytes;
                        association.rx_bytes_delta = association.rx_bytes;
                        association.tx_packets_delta = association.tx_packets;
                        association.rx_packets_delta = association.rx_packets;
                        association.tx_failed_delta = association.tx_failed;
                        association.tx_retries_delta = association.tx_retries;
                        association.tx_duration_delta = association.tx_duration;
                    }

                    association.tx_bytes_bw = safe_div( association.tx_bytes_delta , time_lapse );
                    association.rx_bytes_bw = safe_div( association.rx_bytes_delta , time_lapse );
                    association.tx_packets_bw = safe_div( association.tx_packets_delta , time_lapse );
                    association.rx_packets_bw = safe_div( association.rx_packets_delta , time_lapse );
                    association.tx_failed_pct = safe_pct( association.tx_failed_delta, association.tx_packets);
                    association.tx_retries_pct = safe_pct( association.tx_retries_delta, association.tx_packets);
                    association.tx_duration_pct = safe_pct( (association.tx_duration_delta) / 1000000 , time_lapse );

                    ssid.tx_failed_pct.max = std::max(ssid.tx_failed_pct.max, association.tx_failed_pct);
                    ssid.tx_failed_pct.min = std::min(ssid.tx_failed_pct.min, association.tx_failed_pct);

                    ssid.tx_retries_pct.max = std::max(ssid.tx_retries_pct.max, association.tx_retries_pct);
                    ssid.tx_retries_pct.min = std::min(ssid.tx_retries_pct.min, association.tx_retries_pct);

                    ssid.tx_duration_pct.max = std::max(ssid.tx_duration_pct.max, association.tx_duration_pct);
                    ssid.tx_duration_pct.min = std::min(ssid.tx_duration_pct.min, association.tx_duration_pct);

                    ssid.tx_bytes_bw.max = std::max(ssid.tx_bytes_bw.max, association.tx_bytes_bw);
                    ssid.tx_bytes_bw.min = std::min(ssid.tx_bytes_bw.min, association.tx_bytes_bw);

                    ssid.rx_bytes_bw.max = std::max(ssid.rx_bytes_bw.max, association.rx_bytes_bw);
                    ssid.rx_bytes_bw.min = std::min(ssid.rx_bytes_bw.min, association.rx_bytes_bw);

                    ssid.tx_packets_bw.max = std::max(ssid.tx_packets_bw.max, association.tx_packets_bw);
                    ssid.tx_packets_bw.min = std::min(ssid.tx_packets_bw.min, association.tx_packets_bw);

                    ssid.rx_packets_bw.max = std::max(ssid.rx_packets_bw.max, association.rx_packets_bw);
                    ssid.rx_packets_bw.min = std::min(ssid.rx_packets_bw.min, association.rx_packets_bw);
                }

                ssid.tx_bytes_bw.avg = Average(&AnalyticsObjects::UETimePoint::tx_bytes_bw,ssid.associations);
                ssid.rx_bytes_bw.avg = Average(&AnalyticsObjects::UETimePoint::rx_bytes_bw,ssid.associations);
                ssid.tx_packets_bw.avg = Average(&AnalyticsObjects::UETimePoint::tx_packets_bw,ssid.associations);
                ssid.rx_packets_bw.avg = Average(&AnalyticsObjects::UETimePoint::rx_packets_bw,ssid.associations);
                ssid.tx_failed_pct.avg = Average(&AnalyticsObjects::UETimePoint::tx_failed_pct,ssid.associations);
                ssid.tx_retries_pct.avg = Average(&AnalyticsObjects::UETimePoint::tx_retries_pct,ssid.associations);
                ssid.tx_duration_pct.avg = Average(&AnalyticsObjects::UETimePoint::tx_duration_pct,ssid.associations);
            }

            if (got_connection && got_health) {
                db_DTP.id = MicroService::instance().CreateUUID();
                db_DTP.boardId = boardId_;
                db_DTP.serialNumber = db_DTP.device_info.serialNumber;
                StorageService()->TimePointsDB().CreateRecord(db_DTP);
            }
            tp_base_ = DTP;
        } else {
            tp_base_ = DTP;
            got_base = true;
        }
    }

    void AP::UpdateConnection(const std::shared_ptr<nlohmann::json> &Connection) {
        DI_.pings++;
        DI_.lastContact = OpenWifi::Now();
        try {
            if (Connection->contains("ping")) {
                got_connection = true;
                poco_debug(Logger(),fmt::format("{}: ping message.", DI_.serialNumber));
                DI_.connected = true;
                DI_.lastPing = OpenWifi::Now();
                auto ping = (*Connection)["ping"];
                GetJSON("compatible", ping, DI_.deviceType, std::string{} );
                GetJSON("connectionIp", ping, DI_.connectionIp, std::string{} );
                GetJSON("locale", ping, DI_.locale, std::string{} );
                GetJSON("timestamp", ping, DI_.lastConnection, (uint64_t) OpenWifi::Now() );
                if (ping.contains("firmware")) {
                    auto NewFirmware = ping["firmware"];
                    if (NewFirmware != DI_.lastFirmware) {
                        DI_.lastFirmware = NewFirmware;
                        DI_.lastFirmwareUpdate = OpenWifi::Now();
                    }
                }
            } else if (Connection->contains("disconnection")) {
                poco_debug(Logger(),fmt::format("{}: disconnection message.", DI_.serialNumber));
                auto Disconnection = (*Connection)["disconnection"];
                GetJSON("timestamp", Disconnection, DI_.lastDisconnection, (uint64_t)0 );
                got_base = got_health = got_connection = false;
                DI_.connected = false;
            } else if (Connection->contains("capabilities")) {
                poco_debug(Logger(),fmt::format("{}: connection message.", DI_.serialNumber));
                got_connection = true;
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
            Logger().information(fmt::format("{}: error parsing connection message.", DI_.serialNumber));
        }
    }

    void AP::UpdateHealth(const std::shared_ptr<nlohmann::json> & Health) {
        try {
            got_health = true;
            GetJSON("timestamp", *Health, DI_.lastHealth, (uint64_t)0 );
            GetJSON("sanity", *Health, DI_.health, (uint64_t)0 );
            poco_debug(Logger(),fmt::format("{}: health message.", DI_.serialNumber));
        } catch(...) {
            Logger().information(fmt::format("{}: error parsing health message.", DI_.serialNumber));
        }
    }
}