//
// Created by stephane bourque on 2022-03-10.
//

#pragma once
#include "framework/MicroService.h"

namespace OpenWifi {
    class StateMessage : public Poco::Notification {
    public:
        explicit StateMessage(const std::string &Key, const std::string &Payload ) :
                Key_(Key),
                Payload_(Payload) {}
        const std::string & Key() { return Key_; }
        const std::string & Payload() { return Payload_; }
    private:
        std::string     Key_;
        std::string     Payload_;
    };

    class VenueWatcher;

    class StateReceiver : public SubSystemServer, Poco::Runnable {
    public:

        static auto instance() {
            static auto instance_ = new StateReceiver;
            return instance_;
        }

        int Start() override;
        void Stop() override;
        void StateReceived( const std::string & Key, const std::string & Payload);
        void run() override;
        void Register(uint64_t SerialNumber, VenueWatcher *VW);
        void DeRegister(uint64_t SerialNumber, VenueWatcher *VW);

    private:
        // map of mac(as int), list of (id,func)
        std::map<uint64_t , std::list< VenueWatcher * >>   Notifiers_;
        uint64_t                                StateWatcherId_=0;
        Poco::NotificationQueue                 Queue_;
        Poco::Thread                            Worker_;
        std::atomic_bool                        Running_=false;

        StateReceiver() noexcept:
                SubSystemServer("StatsReceiver", "STATS-RECEIVER", "stats.receiver")
                {
                }
    };

    inline auto StateReceiver() { return StateReceiver::instance(); }

}