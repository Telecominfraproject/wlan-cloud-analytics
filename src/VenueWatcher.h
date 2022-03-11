//
// Created by stephane bourque on 2022-03-10.
//

#pragma once

#include "framework/MicroService.h"

namespace OpenWifi {

    class VenueMessage : public Poco::Notification {
    public:
        explicit VenueMessage(uint64_t SerialNumber, std::shared_ptr<nlohmann::json> &M ) :
                Payload_(M),
                SerialNumber_(SerialNumber) {
        }
        inline std::shared_ptr<nlohmann::json> & Payload() { return Payload_; }
        inline auto SerialNumber() { return SerialNumber_; }
    private:
        std::shared_ptr<nlohmann::json> &Payload_;
        uint64_t SerialNumber_=0;
    };

    class VenueWatcher : public Poco::Runnable {
    public:
        explicit VenueWatcher(const std::string &id, Poco::Logger &L, const std::vector<uint64_t> & SerialNumbers) :
            Id_(id),
            Logger_(L),
            SerialNumbers_(SerialNumbers) {
            std::sort(SerialNumbers_.begin(),SerialNumbers_.end());
            auto last = std::unique(SerialNumbers_.begin(),SerialNumbers_.end());
            SerialNumbers_.erase(last,SerialNumbers_.end());
        }

        void Post(uint64_t SerialNumber, std::shared_ptr<nlohmann::json> &Msg) {
            std::lock_guard G(Mutex_);
            Queue_.enqueueNotification(new VenueMessage(SerialNumber, Msg));
        }

        void Start();
        void Stop();

        void run() final;
        inline Poco::Logger & Logger() { return Logger_; }
        void SetSerialNumbers(const std::vector<uint64_t> &SerialNumbers);

    private:
        std::recursive_mutex    Mutex_;
        std::string             Id_;
        Poco::NotificationQueue Queue_;
        Poco::Logger            &Logger_;
        Poco::Thread            Worker_;
        std::atomic_bool        Running_=false;
        std::vector<uint64_t>   SerialNumbers_;
    };

}