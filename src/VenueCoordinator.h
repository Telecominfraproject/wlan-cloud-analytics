//
// Created by stephane bourque on 2022-03-10.
//

#pragma once
#include "framework/MicroService.h"
#include "VenueWatcher.h"

namespace OpenWifi {

    class VenueCoordinator : public SubSystemServer, Poco::Runnable {
    public:

        static auto instance() {
            static auto instance_ = new VenueCoordinator;
            return instance_;
        }

        int Start() override;
        void Stop() override;
        void run() override;

        void StopBoard(const std::string &id);
        void ModifyBoard(const std::string &id);
        void AddBoard(const std::string &id);

        bool GetDevicesForBoard(const AnalyticsObjects::BoardInfo &B, std::vector<uint64_t> & Devices, bool & VenueExists);
        void GetDevices(std::string &id, AnalyticsObjects::DeviceInfoList & DIL);
        void GetBoardList();
        bool Watching(const std::string &id);
        void RetireBoard(const AnalyticsObjects::BoardInfo &B);

    private:
        Poco::Thread                                        Worker_;
        std::atomic_bool                                    Running_=false;
        std::set<AnalyticsObjects::BoardInfo>               BoardsToWatch_;
        std::map<std::string,std::shared_ptr<VenueWatcher>> Watchers_;

        std::map<std::string,std::vector<uint64_t>>            ExistingBoards_;

        VenueCoordinator() noexcept:
                SubSystemServer("VenueCoordinator", "VENUE-COORD", "venue.coordinator")
                {
                }

        bool StartBoard(const AnalyticsObjects::BoardInfo &B);

    };
    inline auto VenueCoordinator() { return VenueCoordinator::instance(); }

}