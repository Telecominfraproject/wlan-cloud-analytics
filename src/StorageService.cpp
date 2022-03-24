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

        BoardsDB_ = std::make_unique<OpenWifi::BoardsDB>(dbType_,*Pool_, Logger());
        BoardsDB_->Create();
        TimePointsDB_ = std::make_unique<OpenWifi::TimePointDB>(dbType_,*Pool_, Logger());
        TimePointsDB_->Create();

        Updater_.start(*this);

        TimerCallback_ = std::make_unique<Poco::TimerCallback<Storage>>(*this,&Storage::onTimer);
        Timer_.setStartInterval( 20 * 1000);  // first run in 20 seconds
        Timer_.setPeriodicInterval(1 * 60 * 60 * 1000); // 1 hours
        Timer_.start(*TimerCallback_);

        return 0;
    }

    void Storage::onTimer(Poco::Timer &timer) {
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