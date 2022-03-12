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

        void StopVenue(const std::string &id);
        void ModifyVenue(const std::string &id);
        void AddVenue(const std::string &id);

    private:
        Poco::Thread                                Worker_;
        std::atomic_bool                            Running_=false;
        std::vector<std::shared_ptr<VenueWatcher>>  Watchers_;

        VenueCoordinator() noexcept:
                SubSystemServer("VenueCoordinator", "VENUE-COORD", "venue.coordinator")
                {
                }
    };
    inline auto VenueCoordinator() { return VenueCoordinator::instance(); }

}