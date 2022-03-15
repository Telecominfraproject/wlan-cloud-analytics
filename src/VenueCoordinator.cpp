//
// Created by stephane bourque on 2022-03-10.
//

#include "VenueCoordinator.h"
#include "VenueWatcher.h"

namespace OpenWifi {

    int VenueCoordinator::Start() {

        std::vector<std::string>    S{
            "903cb3bb1ef4",
            "04f8f8fc3772",
            "04f8f8fc3b02",
            "24f5a207a130",
            "68215f0427da"
        };

        std::vector<uint64_t> Numbers;
        for(const auto &i:S)
            Numbers.push_back(Utils::SerialNumberToInt(i));

        std::sort(Numbers.begin(),Numbers.end());

        Watchers_["id"] = std::make_shared<VenueWatcher>(std::string{"id-1"},Logger(),Numbers);

        for(const auto &[_,watcher]:Watchers_)
            watcher->Start();

        Worker_.start(*this);
        return 0;
    }

    void VenueCoordinator::Stop() {
        Running_=false;
        Worker_.wakeUp();
        Worker_.join();
    }

    void VenueCoordinator::run() {
        Running_=true;
        while(Running_) {
            Poco::Thread::trySleep(2000);
        }
    }

    void VenueCoordinator::StopVenue(const std::string &id) {

    }

    void VenueCoordinator::ModifyVenue(const std::string &id) {

    }

    void VenueCoordinator::AddVenue(const std::string &id) {

    }

    void VenueCoordinator::GetDevices(std::string &id, AnalyticsObjects::DeviceInfoList &DIL) {
        std::lock_guard     G(Mutex_);

        auto it = Watchers_.find(id);
        if(it!=end(Watchers_)) {
            it->second->GetDevices(DIL.devices);
        }
    }
}