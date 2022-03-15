//
// Created by stephane bourque on 2022-03-10.
//

#include "VenueCoordinator.h"
#include "VenueWatcher.h"
#include "StorageService.h"
#include "sdks/SDK_prov.h"

namespace OpenWifi {

    int VenueCoordinator::Start() {

        auto F = [&](const AnalyticsObjects::BoardInfo &B) ->bool {
            BoardsToWatch_.insert(B);
            Logger().information(Poco::format("Starting watch for %s.", B.info.name));
            return true;
        };
        StorageService()->BoardsDB().Iterate(F);
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

            if(!Running_)
                break;

            std::lock_guard         G(Mutex_);
            if(!BoardsToWatch_.empty()) {
                for(auto board_to_start=BoardsToWatch_.begin(); board_to_start!=BoardsToWatch_.end(); ) {
                    if(StartBoard(*board_to_start)) {
                        board_to_start = BoardsToWatch_.erase(board_to_start);
                    } else {
                        ++board_to_start;
                    }
                }
            }

        }
    }

    bool GetDevicesForBoard(const AnalyticsObjects::BoardInfo &B, std::vector<uint64_t> & Devices) {
        ProvObjects::VenueDeviceList    VDL;
        if(SDK::Prov::Venue::GetDevices(nullptr,B.venueList[0].id,B.venueList[0].monitorSubVenues, VDL)) {
            Devices.clear();
            for(const auto &device:VDL.devices) {
                Devices.push_back(Utils::SerialNumberToInt(device));
            }

            std::sort(Devices.begin(),Devices.end());
            auto LastDevice = std::unique(Devices.begin(),Devices.end());
            Devices.erase(LastDevice,Devices.end());
            return true;
        }
        return false;
    }

    bool VenueCoordinator::StartBoard(const AnalyticsObjects::BoardInfo &B) {
        if(B.venueList.empty())
            return true;

        std::vector<uint64_t>   Devices;
        if(GetDevicesForBoard(B,Devices)) {
            ExistingBoards_[B.info.id] = Devices;
            Watchers_[B.info.id] = std::make_shared<VenueWatcher>(B.info.id,Logger(),Devices);
            Watchers_[B.info.id]->Start();
            Logger().information(Poco::format("Started board %s",B.info.name));
            return true;
        }
        Logger().information(Poco::format("Could not start board %s",B.info.name));
        return false;
    }

    void VenueCoordinator::StopBoard(const std::string &id) {
        std::lock_guard     G(Mutex_);

        auto it = Watchers_.find(id);
        if(it!=Watchers_.end()) {
            it->second->Stop();
            Watchers_.erase(it);
        }
    }

    void VenueCoordinator::ModifyBoard(const std::string &id) {
        AnalyticsObjects::BoardInfo B;
        if(StorageService()->BoardsDB().GetRecord("id",id,B)) {
            std::vector<uint64_t>   Devices;
            if(GetDevicesForBoard(B,Devices)) {

                auto it = ExistingBoards_.find(id);
                if(it!=ExistingBoards_.end()) {
                    if(it->second!=Devices) {
                        auto it2 = Watchers_.find(id);
                        if(it2!=Watchers_.end())
                            it2->second->ModifySerialNumbers(Devices);
                        ExistingBoards_[id] = Devices;
                        Logger().information(Poco::format("Modified board %s",B.info.name));
                    } else {
                        Logger().information(Poco::format("No device changes in board %s",B.info.name));
                    }
                }
                Logger().information(Poco::format("Modified board %s",B.info.name));
                return;
            }
            Logger().information(Poco::format("Could not start board %s",B.info.name));
        }
    }

    void VenueCoordinator::AddBoard(const std::string &id) {
        std::lock_guard     G(Mutex_);

        AnalyticsObjects::BoardInfo B;
        if(StorageService()->BoardsDB().GetRecord("id",id,B))
            BoardsToWatch_.insert(B);
        else
            Logger().information(Poco::format("Board %d does not seem to exist",id));
    }

    void VenueCoordinator::GetDevices(std::string &id, AnalyticsObjects::DeviceInfoList &DIL) {
        std::lock_guard     G(Mutex_);

        auto it = Watchers_.find(id);
        if(it!=end(Watchers_)) {
            it->second->GetDevices(DIL.devices);
        }
    }
}