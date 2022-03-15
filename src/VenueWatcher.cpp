//
// Created by stephane bourque on 2022-03-10.
//

#include "VenueWatcher.h"
#include "StateReceiver.h"
#include "DeviceStatusReceiver.h"
#include "HealthReceiver.h"

namespace OpenWifi {

    void VenueWatcher::Start() {
        for(const auto &mac:SerialNumbers_) {
            auto ap = std::make_shared<AP>(mac);
            APs_[mac ] = ap;
        }

        for(const auto &i:SerialNumbers_)
            StateReceiver()->Register(i,this);

        DeviceStatusReceiver()->Register(SerialNumbers_,this);
        HealthReceiver()->Register(SerialNumbers_,this);
        Worker_.start(*this);
    }

    void VenueWatcher::Stop() {
        Running_ = false;
        Queue_.wakeUpAll();
        Worker_.join();
        DeviceStatusReceiver()->DeRegister(this);
        HealthReceiver()->DeRegister(this);
    }

    void VenueWatcher::run() {
        Running_ = true;
        Poco::AutoPtr<Poco::Notification>	Msg(Queue_.waitDequeueNotification());
        while(Msg && Running_) {
            auto MsgContent = dynamic_cast<VenueMessage *>(Msg.get());
            if(MsgContent!= nullptr) {
                try {
                    auto State = MsgContent->Payload();
                    if(MsgContent->Type()==VenueMessage::connection) {
                        auto It = APs_.find(MsgContent->SerialNumber());
                        if(It!=end(APs_)) {
                            It->second->UpdateConnection(MsgContent->Payload());
                        }
                    } else if(MsgContent->Type()==VenueMessage::state) {
                        auto It = APs_.find(MsgContent->SerialNumber());
                        if (It != end(APs_)) {
                            It->second->UpdateStats(MsgContent->Payload());
                        }
                    } else if(MsgContent->Type()==VenueMessage::health) {
                        auto It = APs_.find(MsgContent->SerialNumber());
                        if (It != end(APs_)) {
                            It->second->UpdateHealth(MsgContent->Payload());
                        }
                    }
                } catch (const Poco::Exception &E) {
                    Logger().log(E);
                } catch (...) {

                }
            } else {

            }
            Msg = Queue_.waitDequeueNotification();
        }
    }

    void VenueWatcher::ModifySerialNumbers(const std::vector<uint64_t> &SerialNumbers) {
        std::lock_guard G(Mutex_);

        std::vector<uint64_t>  Diff;
        std::set_symmetric_difference(SerialNumbers_.begin(),SerialNumbers_.end(),SerialNumbers.begin(),
                                      SerialNumbers.end(),std::inserter(Diff,Diff.begin()));

        std::vector<uint64_t>  ToRemove;
        std::set_intersection(SerialNumbers_.begin(),SerialNumbers_.end(),Diff.begin(),
                              Diff.end(),std::inserter(ToRemove,ToRemove.begin()));

        std::vector<uint64_t>  ToAdd;
        std::set_intersection(SerialNumbers.begin(),SerialNumbers.end(),Diff.begin(),
                              Diff.end(),std::inserter(ToAdd,ToAdd.begin()));

        for(const auto &i:ToRemove)
            StateReceiver()->DeRegister(i,this);
        for(const auto &i:ToAdd)
            StateReceiver()->Register(i,this);

        SerialNumbers_ = SerialNumbers;
    }

    void VenueWatcher::GetDevices(std::vector<AnalyticsObjects::DeviceInfo> & DIL) {
        std::lock_guard G(Mutex_);

        DIL.reserve(APs_.size());
        for(const auto &[serialNumber,DI]:APs_)
            DIL.push_back(DI->Info());
    }

}