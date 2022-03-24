//
// Created by stephane bourque on 2022-03-10.
//

#include "StateReceiver.h"
#include "VenueWatcher.h"
#include "fmt/core.h"

namespace OpenWifi {

    int StateReceiver::Start() {
        Running_ = true;
        Types::TopicNotifyFunction F = [this](const std::string &Key, const std::string &Payload) { this->StateReceived(Key,Payload); };
        StateWatcherId_ = KafkaManager()->RegisterTopicWatcher(KafkaTopics::STATE, F);
        Worker_.start(*this);
        return 0;
    };

    void StateReceiver::Stop() {
        Running_ = false;
        KafkaManager()->UnregisterTopicWatcher(KafkaTopics::STATE, StateWatcherId_);
        Queue_.wakeUpAll();
        Worker_.join();
    };

    void StateReceiver::run() {
        Poco::AutoPtr<Poco::Notification>	Note(Queue_.waitDequeueNotification());
        while(Note && Running_) {
            auto Msg = dynamic_cast<StateMessage *>(Note.get());
            if(Msg!= nullptr) {
                try {
                    nlohmann::json msg = nlohmann::json::parse(Msg->Payload());
                    if (msg.contains(uCentralProtocol::PAYLOAD)) {
                        auto payload = msg[uCentralProtocol::PAYLOAD];
                        if (payload.contains("state") && payload.contains("serial")) {
                            auto serialNumber = payload["serial"].get<std::string>();
                            auto state = std::make_shared<nlohmann::json>(payload["state"]);
                            std::lock_guard     G(Mutex_);
                            auto it = Notifiers_.find(Utils::SerialNumberToInt(serialNumber));
                            if(it!=Notifiers_.end()) {
                                for(const auto &i:it->second) {
                                    i->PostState(Utils::SerialNumberToInt(serialNumber), state);
                                }
                            }
                        }
                    }
                } catch (const Poco::Exception &E) {
                    Logger().log(E);
                } catch (...) {

                }
            } else {

            }
            Note = Queue_.waitDequeueNotification();
        }
    }

    void StateReceiver::StateReceived( const std::string & Key, const std::string & Payload) {
        std::lock_guard G(Mutex_);
        Logger().information(fmt::format("Device({}): State message.", Key));
        Queue_.enqueueNotification( new StateMessage(Key,Payload));
    }

    void StateReceiver::Register(uint64_t SerialNumber, VenueWatcher *VW) {
        std::lock_guard     G(Mutex_);
        auto it = Notifiers_.find(SerialNumber);
        if(it == Notifiers_.end()) {
            std::list<VenueWatcher *> L;
            L.push_back(VW);
            Notifiers_[SerialNumber] = L;
        } else {
            it->second.push_back(VW);
        }
    }

    void StateReceiver::DeRegister(uint64_t SerialNumber, VenueWatcher *VW) {
        std::lock_guard     G(Mutex_);
        auto it = Notifiers_.find(SerialNumber);
        if(it==Notifiers_.end())
            return;
        for(auto i=it->second.begin();i!=it->second.end();i++) {
            if(*i==VW) {
                it->second.erase(i);
                break;
            }
        }
    }



}