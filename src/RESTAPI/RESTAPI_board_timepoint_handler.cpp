//
// Created by stephane bourque on 2022-03-21.
//

#include "RESTAPI_board_timepoint_handler.h"
#include "StorageService.h"

#include <algorithm>

namespace OpenWifi {
    static bool count_compare(const std::pair<std::string,std::uint64_t> &lhs, const std::pair<std::string,std::uint64_t> &rhs) {
        return lhs.second < rhs.second;
    }

    static bool min_point_timestamp(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) {
        return lhs.timestamp < rhs.timestamp;
    }

    typedef std::vector<std::pair<std::uint64_t , std::uint64_t >>          bucket_timespans;
    static std::uint64_t find_number_of_buckets(const std::vector<AnalyticsObjects::DeviceTimePoint> &points, bucket_timespans &  buckets) {
        std::map<std::string,uint64_t> counts;
        for(const auto &point:points) {
            auto hint = counts.find(point.serialNumber);
            if(hint == end(counts))
                counts[point.serialNumber]=1;
            else
                hint->second++;
        }
        auto max_buckets = std::max_element(counts.begin(),counts.end(), count_compare);
        auto min_timestamp = std::min_element(points.begin(),points.end(),min_point_timestamp);
        auto max_timestamp = std::max_element(points.begin(),points.end(),min_point_timestamp);

        std::cout << "Max-buckets: " << max_buckets->second << " min timestamp:" << min_timestamp->timestamp << " max timestamp:" << max_timestamp->timestamp << " interval: " << ((max_timestamp-min_timestamp) / max_buckets->second) << std::endl;


        return max_buckets->second;
    }

    typedef std::vector< std::vector<AnalyticsObjects::DeviceTimePoint>>    split_points;

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
            if((v.ap_data.*T)!=0) {
                P.min = std::min(P.min, (v.ap_data.*T));
            }
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
                if((radio.*T)!=0) {
                    P.min = std::min((double) P.min, (double) (radio.*T));
                }
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
        if(id.empty() || !Utils::ValidUUID(id)) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);
        std::uint64_t maxRecords;
        if(Request->has("limit"))
            maxRecords = QB_.Limit;
        else
            maxRecords = GetParameter("maxRecords",1000);

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
        std::cout << "1 MaxRecords=" << maxRecords << " retrieved=" << Points->points.size() << std::endl;

        // sort by timestamp & serial number.
        struct {
            bool operator()(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) const {
                if (lhs.device_info.serialNumber < rhs.device_info.serialNumber) return true;
                if (lhs.device_info.serialNumber > rhs.device_info.serialNumber) return false;
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
                return (lhs.station < rhs.station);
            }
        } Association_sort;


        std::sort( Points->points.begin(), Points->points.end(), DeviceTimePoint_sort);
        std::cout << "2 MaxRecords=" << maxRecords << " retrieved=" << Points->points.size() << std::endl;
        bucket_timespans buckets;
        auto BucketsNeeded = find_number_of_buckets(Points->points,buckets);

        split_points sp;
        split_in_buckets(BucketsNeeded,Points->points, sp);
        // must sort each bucket according to serial number.
        for(auto &i: sp) {
            std::sort(i.begin(),i.end(),DeviceTimePoint_sort);
            // now sort according to UEs within a block
            for(auto &j:i) {
                std::sort(j.ssid_data.begin(),j.ssid_data.end(),SSID_sort);
                for(auto &k:j.ssid_data) {
                    std::sort(k.associations.begin(),k.associations.end(),Association_sort);
                }
            }
        }

        Poco::JSON::Object  Answer;
        if(!pointsStatsOnly) {
            Poco::JSON::Array   Points_OuterArray;
            for (const auto &point_list:sp) {
                Poco::JSON::Array Points_InnerArray;
                for (const auto &point: point_list) {
                    Poco::JSON::Object O;
                    point.to_json(O);
                    Points_InnerArray.add(O);
                }
                Points_OuterArray.add(Points_InnerArray);
            }
            Answer.set("points",Points_OuterArray);
        }

        //  calculate the stats for each time slot
        if(!pointsOnly) {
            Poco::JSON::Array   Stats_Array;
            for (const auto &point_list:sp) {
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
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::temperature, point_list, DTPA.temperature);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::tx_power, point_list, DTPA.tx_power);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::active_pct, point_list, DTPA.active_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::busy_pct, point_list, DTPA.busy_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::receive_pct, point_list, DTPA.receive_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::transmit_pct, point_list, DTPA.transmit_pct);

                Poco::JSON::Object Stats_point;
                DTPA.to_json(Stats_point);
                Stats_Array.add(Stats_point);
            }
            Answer.set("stats", Stats_Array);
        }

        return ReturnObject(Answer);
    }

    void RESTAPI_board_timepoint_handler::DoDelete() {
        auto id = GetBinding("id","");
        if(id.empty() || !Utils::ValidUUID(id)) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        auto fromDate = GetParameter("fromDate",0);
        auto endDate = GetParameter("endDate",0);

        StorageService()->TimePointsDB().DeleteTimeLine(id,fromDate,endDate);
        return OK();
    }

}