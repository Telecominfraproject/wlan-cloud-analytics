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

          private:
            Poco::Thread                                        Updater_;
            std::atomic_bool                                    Running_=false;
   };
   inline auto StorageService() { return Storage::instance(); }
}  // namespace

