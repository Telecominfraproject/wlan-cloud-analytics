//
// Created by stephane bourque on 2022-03-21.
//

#include "RESTAPI_board_timepoint_handler.h"
#include "StorageService.h"

namespace OpenWifi {
    static auto find_number_of_buckets(std::vector<AnalyticsObjects::DeviceTimePoint> &p) {
        uint32_t buckets=0,cur_buckets=0;
        std::string current_serialNumber;
        for(const auto &i:p) {
            if(current_serialNumber.empty()) {
                current_serialNumber = i.device_info.serialNumber;
                cur_buckets=0;
            }
            if(current_serialNumber==i.device_info.serialNumber) {
                cur_buckets++;
            } else {
                buckets = std::max(buckets,cur_buckets);
                current_serialNumber=i.device_info.serialNumber;
                cur_buckets=1;
            }
        }
        return std::max(buckets,cur_buckets);
    }

    typedef std::vector< std::vector<AnalyticsObjects::DeviceTimePoint>> split_points;

    static void split_in_buckets([[maybe_unused]] uint32_t buckets,std::vector<AnalyticsObjects::DeviceTimePoint> &p,split_points &sp) {
        std::string cur_sn;
        uint32_t cur_bucket=0;
        for(const auto &i:p) {
            if(cur_sn.empty()) {
                cur_bucket=1;
                cur_sn=i.device_info.serialNumber;
            }
            if(cur_sn==i.device_info.serialNumber) {
                if (cur_bucket>sp.size()) {
                    std::vector<AnalyticsObjects::DeviceTimePoint>  tmp_p;
                    tmp_p.push_back(i);
                    sp.push_back(tmp_p);
                    cur_bucket++;
                } else {
                    sp[cur_bucket-1].push_back(i);
                    cur_bucket++;
                }
            } else {
                cur_bucket=1;
                sp[cur_bucket-1].push_back(i);
                cur_bucket++;
                cur_sn=i.device_info.serialNumber;
            }
        }
    }

    template <typename X, typename M> void AverageAPData( X T, const std::vector<M> &Values, AnalyticsObjects::AveragePoint &P) {
        if(Values.empty())
            return;
        double sum = 0.0;
        for(const auto &v:Values) {
            sum += (v.ap_data.*T);
            P.min = std::min(P.min,(v.ap_data.*T));
            P.max = std::min(P.max,(v.ap_data.*T));
        }
        P.avg = sum / (double) Values.size();
    }

    template <typename X, typename M> void AverageRadioData( X T, const std::vector<M> &Values, AnalyticsObjects::AveragePoint &P) {
        if(Values.empty())
            return;
        double sum = 0.0;
        uint32_t num_values = 0;
        for(const auto &value:Values) {
            for(const auto &radio:value.radio_data) {
                num_values++;
                sum += (radio.*T);
                P.min = std::min((double)P.min, (double)(radio.*T));
                P.max = std::max((double)P.max, (double)(radio.*T));
            }
        }
        if(num_values)
            P.avg = sum / (double) num_values;
        else
            P.avg = 0.0;
    }

    void RESTAPI_board_timepoint_handler::DoGet() {
        auto id = GetBinding("id","");
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);
        auto maxRecords = GetParameter("maxRecords",1000);
        auto statsOnly = GetBoolParameter("statsOnly");
        auto pointsOnly = GetBoolParameter("pointsOnly");
        auto pointsStatsOnly = GetBoolParameter("pointsStatsOnly");

        if(statsOnly) {
            AnalyticsObjects::DeviceTimePointStats  DTPS;
            Poco::JSON::Object  Answer;
            DB_.GetStats(id,DTPS);
            DTPS.to_json(Answer);
            return ReturnObject(Answer);
        }

        auto Points = std::make_unique<AnalyticsObjects::DeviceTimePointList>();
        StorageService()->TimePointsDB().SelectRecords(id,fromDate, endDate, maxRecords, Points->points);

        // sort by timestamp & serial number.
        struct {
            bool operator()(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) const {
                if(lhs.device_info.serialNumber < rhs.device_info.serialNumber) return true;
                if(lhs.device_info.serialNumber > rhs.device_info.serialNumber) return false;
                return lhs.timestamp < rhs.timestamp;
            }
        } DeviceTimePoint_sort;

        struct {
            bool operator()(const AnalyticsObjects::SSIDTimePoint &lhs, const AnalyticsObjects::SSIDTimePoint &rhs) const {
                if(lhs.ssid < rhs.ssid) return true;
                if(lhs.ssid > rhs.ssid) return false;
                return lhs.bssid < rhs.bssid;
            }
        } SSID_sort;

        struct {
            bool operator()(const AnalyticsObjects::UETimePoint &lhs, const AnalyticsObjects::UETimePoint &rhs) const {
                if(lhs.station < rhs.station) return true;
                return false;
            }
        } Association_sort;

        std::sort( Points->points.begin(), Points->points.end(), DeviceTimePoint_sort);
        auto BucketsNeeded = find_number_of_buckets(Points->points);

        auto sp = std::make_unique<split_points>();
        split_in_buckets(BucketsNeeded,Points->points, *sp);
        // must sort each bucket according to serial number.
        for(auto &i: *sp) {
            std::sort(i.begin(),i.end(),DeviceTimePoint_sort);
            // now sort according to UEs within a block
            for(auto &j:i) {
                std::sort(j.ssid_data.begin(),j.ssid_data.end(),SSID_sort);
                for(auto &k:j.ssid_data) {
                    std::sort(k.associations.begin(),k.associations.end(),Association_sort);
                }
            }
        }

        auto Answer = std::make_unique<Poco::JSON::Object>();
        if(!pointsStatsOnly) {
            auto Points_OuterArray = std::make_unique<Poco::JSON::Array>();
            for (const auto &point_list:*sp) {
                Poco::JSON::Array Points_InnerArray;
                for (const auto &point: point_list) {
                    Poco::JSON::Object O;
                    point.to_json(O);
                    Points_InnerArray.add(O);
                }
                Points_OuterArray->add(Points_InnerArray);
            }
            Answer->set("points",*Points_OuterArray);
        }

        //  calculate the stats for each time slot
        if(!pointsOnly) {
            auto Stats_Array = std::make_unique<Poco::JSON::Array>();
            for (const auto &point_list:*sp) {
                AnalyticsObjects::DeviceTimePointAnalysis DTPA;

                DTPA.timestamp = point_list[0].timestamp;
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_bytes_bw, point_list, DTPA.tx_bytes_bw);
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_bytes_bw, point_list, DTPA.rx_bytes_bw);
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_dropped_pct, point_list, DTPA.rx_dropped_pct);
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_dropped_pct, point_list, DTPA.tx_dropped_pct);
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_packets_bw, point_list, DTPA.rx_packets_bw);
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_packets_bw, point_list, DTPA.tx_packets_bw);
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_errors_pct, point_list, DTPA.rx_errors_pct);
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_errors_pct, point_list, DTPA.tx_errors_pct);

                AverageRadioData(&AnalyticsObjects::RadioTimePoint::noise, point_list, DTPA.noise);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::tx_power, point_list, DTPA.tx_power);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::active_pct, point_list, DTPA.active_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::busy_pct, point_list, DTPA.busy_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::receive_pct, point_list, DTPA.receive_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::transmit_pct, point_list, DTPA.transmit_pct);

                Poco::JSON::Object Stats_point;
                DTPA.to_json(Stats_point);
                Stats_Array->add(Stats_point);
            }
            Answer->set("stats", *Stats_Array);
        }

/*        static int f=0;
        std::ostringstream OO;
        Answer.stringify(OO);
        std::ofstream of("msg"+std::to_string(f++)+".json", std::ios_base::trunc );
        of << OO.str();
*/



        return ReturnObject(*Answer);
    }
}