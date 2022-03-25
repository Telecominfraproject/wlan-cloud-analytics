//
// Created by stephane bourque on 2022-03-10.
//

#pragma once

#include "framework/MicroService.h"
#include "APStats.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"

namespace OpenWifi {

    class VenueMessage : public Poco::Notification {
    public:
        enum MsgType {
            connection,
            state,
            health
        };

        explicit VenueMessage(uint64_t SerialNumber, MsgType Msg, std::shared_ptr<nlohmann::json> &M ) :
                Payload_(M),
                Type_(Msg),
                SerialNumber_(SerialNumber) {
        }
        inline std::shared_ptr<nlohmann::json> & Payload() { return Payload_; }
        inline auto SerialNumber() { return SerialNumber_; }
        inline uint64_t Type() { return Type_; }

    private:
        std::shared_ptr<nlohmann::json> Payload_;
        MsgType                         Type_;
        uint64_t                        SerialNumber_=0;
    };

    class VenueWatcher : public Poco::Runnable {
    public:
        explicit VenueWatcher(const std::string &boardId, Poco::Logger &L, const std::vector<uint64_t> & SerialNumbers) :
                boardId_(boardId),
                Logger_(L),
                SerialNumbers_(SerialNumbers) {
            std::sort(SerialNumbers_.begin(),SerialNumbers_.end());
            auto last = std::unique(SerialNumbers_.begin(),SerialNumbers_.end());
            SerialNumbers_.erase(last,SerialNumbers_.end());
        }

        inline void PostState(uint64_t SerialNumber, std::shared_ptr<nlohmann::json> &Msg) {
            std::lock_guard G(Mutex_);
            Queue_.enqueueNotification(new VenueMessage(SerialNumber, VenueMessage::state, Msg));
        }

        inline void PostConnection(uint64_t SerialNumber, std::shared_ptr<nlohmann::json> &Msg) {
            std::lock_guard G(Mutex_);
            Queue_.enqueueNotification(new VenueMessage(SerialNumber, VenueMessage::connection, Msg));
        }

        inline void PostHealth(uint64_t SerialNumber, std::shared_ptr<nlohmann::json> &Msg) {
            std::lock_guard G(Mutex_);
            Queue_.enqueueNotification(new VenueMessage(SerialNumber, VenueMessage::health, Msg));
        }

        void Start();
        void Stop();

        void run() final;
        inline Poco::Logger & Logger() { return Logger_; }
        void ModifySerialNumbers(const std::vector<uint64_t> &SerialNumbers);
        void GetDevices(std::vector<AnalyticsObjects::DeviceInfo> & DI);

        void GetBandwidth(uint64_t start, uint64_t end, uint64_t interval , AnalyticsObjects::BandwidthAnalysis & BW);

    private:
        std::recursive_mutex                        Mutex_;
        std::string                                 boardId_;
        Poco::NotificationQueue                     Queue_;
        Poco::Logger                                &Logger_;
        Poco::Thread                                Worker_;
        std::atomic_bool                            Running_=false;
        std::vector<uint64_t>                       SerialNumbers_;
        std::map<uint64_t, std::shared_ptr<AP>>     APs_;
    };

}