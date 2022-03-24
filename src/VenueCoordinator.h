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

        void GetDevices(std::string &id, AnalyticsObjects::DeviceInfoList & DIL);

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