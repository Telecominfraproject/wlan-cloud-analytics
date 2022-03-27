//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#pragma once

#include "framework/MicroService.h"
#include "framework/StorageClass.h"
#include "storage/storage_boards.h"
#include "storage/storage_timepoints.h"

namespace OpenWifi {
    class Storage : public StorageClass, Poco::Runnable {
        public:
            static auto instance() {
                static auto instance_ = new Storage;
                return instance_;
            }

            int 	Start() override;
            void 	Stop() override;

            void run() final;
            OpenWifi::BoardsDB & BoardsDB() { return *BoardsDB_; };
            OpenWifi::TimePointDB & TimePointsDB() { return *TimePointsDB_; };
            void onTimer(Poco::Timer & timer);

          private:
            std::unique_ptr<OpenWifi::BoardsDB>                 BoardsDB_;
            std::unique_ptr<OpenWifi::TimePointDB>              TimePointsDB_;
            Poco::Thread                                        Updater_;
            std::atomic_bool                                    Running_=false;
            Poco::Timer                                         Timer_;
            std::unique_ptr<Poco::TimerCallback<Storage>>       TimerCallback_;
            uint64_t                                            PeriodicCleanup_=6*60*60;
   };
   inline auto StorageService() { return Storage::instance(); }
}  // namespace

