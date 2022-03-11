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
            void onTimer(Poco::Timer & timer);

          private:
            std::unique_ptr<OpenWifi::BoardsDB>                 BoardsDB_;
            Poco::Thread                                        Updater_;
            std::atomic_bool                                    Running_=false;
            Poco::Timer                                         Timer_;
            std::unique_ptr<Poco::TimerCallback<Storage>>       TimerCallback_;
   };
   inline auto StorageService() { return Storage::instance(); }
}  // namespace

