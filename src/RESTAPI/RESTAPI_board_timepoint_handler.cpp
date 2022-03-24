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

    static void split_in_buckets(uint32_t buckets,std::vector<AnalyticsObjects::DeviceTimePoint> &p,split_points &sp) {
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
        auto maxRecords = GetParameter("maxRecords",100);
        auto stats = GetBoolParameter("stats");

        if(stats) {
            AnalyticsObjects::DeviceTimePointStats  DTPS;
            Poco::JSON::Object  Answer;
            DB_.GetStats(id,DTPS);
            DTPS.to_json(Answer);
            return ReturnObject(Answer);
        }

        AnalyticsObjects::DeviceTimePointList   Points;
        StorageService()->TimePointsDB().SelectRecords(fromDate, endDate, maxRecords, Points.points);

        // sort by timestamp & serial number.
        struct {
            bool operator()(const AnalyticsObjects::DeviceTimePoint &lhs, const AnalyticsObjects::DeviceTimePoint &rhs) const {
                if(lhs.device_info.serialNumber < rhs.device_info.serialNumber) return true;
                if(lhs.device_info.serialNumber > rhs.device_info.serialNumber) return false;
                return lhs.timestamp < rhs.timestamp;
            }
        } custom_sort;

        std::sort( Points.points.begin(), Points.points.end(), custom_sort);
        auto BucketsNeeded = find_number_of_buckets(Points.points);

        split_points sp;
        split_in_buckets(BucketsNeeded,Points.points,sp);

        Poco::JSON::Object  Answer;
        Poco::JSON::Array   Outer;
        for(const auto &i:sp) {
            Poco::JSON::Array   InnerArray;
            for(const auto &j:i) {
                Poco::JSON::Object  O;
                j.to_json(O);
                InnerArray.add(O);
            }
            Outer.add(InnerArray);
        }
        Answer.set("points",Outer);
        return ReturnObject(Answer);
    }
}