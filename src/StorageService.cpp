//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "StorageService.h"
#include "RESTObjects/RESTAPI_ProvObjects.h"
#include "framework/MicroServiceFuncs.h"
#include "fmt/format.h"
#include "framework/utils.h"

namespace OpenWifi {

    int Storage::Start() {
        poco_notice(Logger(),"Starting...");
		std::lock_guard		Guard(Mutex_);

		StorageClass::Start();

        BoardsDB_ = std::make_unique<OpenWifi::BoardsDB>(dbType_,*Pool_, Logger());
        TimePointsDB_ = std::make_unique<OpenWifi::TimePointDB>(dbType_,*Pool_, Logger());
        WifiClientHistoryDB_ = std::make_unique<OpenWifi::WifiClientHistoryDB>(dbType_,*Pool_, Logger());

        TimePointsDB_->Create();
        BoardsDB_->Create();
        WifiClientHistoryDB_->Create();

        PeriodicCleanup_ = MicroServiceConfigGetInt("storage.cleanup.interval", 6*60*60);
        if(PeriodicCleanup_<1*60*60)
            PeriodicCleanup_ = 1*60*60;

        Updater_.start(*this);

        TimerCallback_ = std::make_unique<Poco::TimerCallback<Storage>>(*this,&Storage::onTimer);
        Timer_.setStartInterval( 60 * 1000);  // first run in 20 seconds
        Timer_.setPeriodicInterval((long)PeriodicCleanup_ * 1000); // 1 hours
        Timer_.start(*TimerCallback_);

        return 0;
    }

    void Storage::onTimer([[maybe_unused]] Poco::Timer &timer) {
        BoardsDB::RecordVec BoardList;
        uint64_t start = 0 ;
        bool done = false;
        const uint64_t batch=100;
        poco_information(Logger(),"Starting cleanup of TimePoint Database");
        while(!done) {
            if(!BoardsDB().GetRecords(start,batch,BoardList)) {
                for(const auto &board:BoardList) {
                    for(const auto &venue:board.venueList) {
                        auto now = OpenWifi::Now();
                        auto lower_bound = now - venue.retention;
                        poco_information(Logger(),fmt::format("Removing old records for board '{}'",board.info.name));
                        BoardsDB().DeleteRecords(fmt::format(" boardId='{}' and timestamp<{}", board.info.id, lower_bound));
                    }
                }
            }
            done = (BoardList.size() < batch);
        }

        auto MaxDays = MicroServiceConfigGetInt("wificlient.age.limit",14);
        auto LowerDate = OpenWifi::Now() - (MaxDays*60*60*24);
        poco_information(Logger(),fmt::format("Removing WiFi Clients history older than {} days.", MaxDays));
        StorageService()->WifiClientHistoryDB().DeleteRecords(fmt::format(" timestamp<{} ", LowerDate));
        poco_information(Logger(),fmt::format("Done cleanup of databases. Next run in {} seconds.", PeriodicCleanup_));
    }

    void Storage::run() {
        Utils::SetThreadName("strg-updtr");
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
        poco_notice(Logger(),"Stopping...");
	    Running_=false;
        Timer_.stop();
	    Updater_.wakeUp();
	    Updater_.join();
        poco_notice(Logger(),"Stopped...");
    }
}

// namespace