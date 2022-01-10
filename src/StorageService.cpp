//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "StorageService.h"
#include "RESTObjects/RESTAPI_ProvObjects.h"

namespace OpenWifi {

    int Storage::Start() {
		std::lock_guard		Guard(Mutex_);

		StorageClass::Start();
        Updater_.start(*this);
        return 0;
    }

    void Storage::run() {
	    Running_ = true ;
	    bool FirstRun=true;
	    long Retry = 2000;
	    while(Running_) {
	        if(!FirstRun)
	            Poco::Thread::trySleep(Retry);
	        if(!Running_)
	            break;
            FirstRun = false;
            Retry = 2000;
	    }
	}

    void Storage::Stop() {
	    Running_=false;
	    Updater_.wakeUp();
	    Updater_.join();
        Logger().notice("Stopping.");
    }
}

// namespace