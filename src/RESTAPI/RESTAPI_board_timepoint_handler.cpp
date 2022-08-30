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

    static void find_number_of_buckets(const std::vector<AnalyticsObjects::DeviceTimePoint> &points, bucket_timespans &  buckets) {
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

        float interval =  (float) ((max_timestamp->timestamp-min_timestamp->timestamp) / max_buckets->second);

        std::cout << "Max-buckets: " << max_buckets->second << " min timestamp:" << min_timestamp->timestamp <<
        " max timestamp:" << max_timestamp->timestamp << " interval: " <<
        interval << std::endl;

        auto buckets_left = max_buckets->second;
        float min,max;
        min = (float) min_timestamp->timestamp;
        max = min + interval;
        for(;buckets_left;buckets_left--) {
            std::pair<std::uint64_t,std::uint64_t>  e;
            e.first = (std::uint64_t) min;
            e.second = (std::uint64_t) max;
            min = (float)e.second+1;
            max = min + interval;
            buckets.emplace_back(e);
        }
    }

    typedef std::vector< std::vector<AnalyticsObjects::DeviceTimePoint>>    split_points;

    static void split_in_buckets(const bucket_timespans & buckets, std::vector<AnalyticsObjects::DeviceTimePoint> &points,split_points &sp) {
        for(auto i = buckets.size();i;--i)
        {
            std::vector<AnalyticsObjects::DeviceTimePoint> v;
            sp.emplace_back(v);
        }

        for(const auto &point:points) {
            uint64_t index=0;
            for(const auto &[lower,upper]:buckets) {
                if(point.timestamp>=lower && point.timestamp<=upper) {
                    sp[index].emplace_back(point);
                    break;
                }
                index++;
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

    static void NewSort(const AnalyticsObjects::DeviceTimePointList &l,split_points &sp) {
        struct {
            bool operator()(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) const {
                if (lhs.timestamp < rhs.timestamp) return true;
                if (lhs.timestamp > rhs.timestamp) return false;
                return lhs.device_info.serialNumber < rhs.device_info.serialNumber;
            }
        } sort_ts_serial;

        struct {
            bool operator()(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) const {
                if (lhs.device_info.serialNumber < rhs.device_info.serialNumber) return true;
                if (lhs.device_info.serialNumber > rhs.device_info.serialNumber) return false;
                return lhs.timestamp < rhs.timestamp;
            }
        } sort_serial_ts;

        // attempt at finding an interval
        AnalyticsObjects::DeviceTimePointList tmp{l};
        std::sort(tmp.points.begin(),tmp.points.end(),sort_serial_ts);

        std::string     cur_ser;
        std::uint64_t   cur_int=0,start_val, last_val, first_val = 0;
        for(const auto &point:tmp.points) {
            if(cur_ser.empty()) {
                start_val = point.timestamp;
                cur_ser = point.serialNumber;
                first_val = point.timestamp;
                continue;
            }

            if(cur_ser==point.serialNumber) {
                auto this_int = point.timestamp - start_val;
                if(cur_int) {
                    if(this_int<cur_int) {
                        cur_int = this_int;
                    }
                } else {
                    cur_int = this_int;
                }
                start_val = point.timestamp;
            } else {
                cur_ser = point.serialNumber;
                start_val = point.timestamp;
            }
            last_val = point.timestamp;
        }

        std::cout << "Intervals: " << cur_int << std::endl;

        std::vector<std::pair<std::uint64_t,std::uint64_t>>     time_slots;         //  timeslot 0 has <t1,t2>
        std::vector<std::set<std::string>>                      serial_numbers;     //  serial number already in a timeslot.


        std::uint64_t cur_first=first_val,cur_end=0;
        sp.clear();
        while(cur_end<last_val) {
            std::pair<std::uint64_t,std::uint64_t>  e;
            e.first = cur_first;
            e.second = e.first + cur_int-1;
            cur_first = e.second+1;
            cur_end = e.second;
            time_slots.emplace_back(e);
            std::set<std::string> q;
            serial_numbers.emplace_back(q);
            std::vector<AnalyticsObjects::DeviceTimePoint>  qq;
            sp.emplace_back(qq);
        }

        for(const auto &point:tmp.points) {
            std::uint64_t slot_index=0;
            for(const auto &slot:time_slots) {
                if(point.timestamp >= slot.first && point.timestamp <= slot.second) {
                    serial_numbers[slot_index].insert(point.serialNumber);
                    sp[slot_index].emplace_back(point);
                }
                slot_index++;
            }
        }

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


        split_points sp;

        NewSort(*Points,sp);

        /*

        std::sort( Points->points.begin(), Points->points.end(), DeviceTimePoint_sort);
        std::cout << "2 MaxRecords=" << maxRecords << " retrieved=" << Points->points.size() << std::endl;
        std::cout << __LINE__ << std::endl;
        bucket_timespans buckets;
        std::cout << __LINE__ << std::endl;
        find_number_of_buckets(Points->points,buckets);
        std::cout << __LINE__ << std::endl;
        std::cout << __LINE__ << std::endl;
        split_in_buckets(buckets,Points->points, sp);
        // must sort each bucket according to serial number.
        std::cout << __LINE__ << std::endl;
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
        */

        std::cout << __LINE__ << std::endl;


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

        std::cout << __LINE__ << std::endl;
        //  calculate the stats for each time slot
        if(!pointsOnly) {
            Poco::JSON::Array   Stats_Array;
            for (const auto &point_list:sp) {
                AnalyticsObjects::DeviceTimePointAnalysis DTPA;

                std::cout << __LINE__ << std::endl;
                if(point_list.empty())
                    continue;

                DTPA.timestamp = point_list[0].timestamp;
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_bytes_bw, point_list, DTPA.tx_bytes_bw);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_bytes_bw, point_list, DTPA.rx_bytes_bw);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_dropped_pct, point_list, DTPA.rx_dropped_pct);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_dropped_pct, point_list, DTPA.tx_dropped_pct);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_packets_bw, point_list, DTPA.rx_packets_bw);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_packets_bw, point_list, DTPA.tx_packets_bw);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::rx_errors_pct, point_list, DTPA.rx_errors_pct);
                std::cout << __LINE__ << std::endl;
                AverageAPData(&AnalyticsObjects::APTimePoint::tx_errors_pct, point_list, DTPA.tx_errors_pct);
                std::cout << __LINE__ << std::endl;

                AverageRadioData(&AnalyticsObjects::RadioTimePoint::noise, point_list, DTPA.noise);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::temperature, point_list, DTPA.temperature);
                std::cout << __LINE__ << std::endl;
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::tx_power, point_list, DTPA.tx_power);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::active_pct, point_list, DTPA.active_pct);
                std::cout << __LINE__ << std::endl;
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::busy_pct, point_list, DTPA.busy_pct);
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::receive_pct, point_list, DTPA.receive_pct);
                std::cout << __LINE__ << std::endl;
                AverageRadioData(&AnalyticsObjects::RadioTimePoint::transmit_pct, point_list, DTPA.transmit_pct);

                std::cout << __LINE__ << std::endl;
                Poco::JSON::Object Stats_point;
                std::cout << __LINE__ << std::endl;
                DTPA.to_json(Stats_point);
                std::cout << __LINE__ << std::endl;
                Stats_Array.add(Stats_point);
                std::cout << __LINE__ << std::endl;
            }
            std::cout << __LINE__ << std::endl;
            Answer.set("stats", Stats_Array);
        }
        std::cout << __LINE__ << std::endl;

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