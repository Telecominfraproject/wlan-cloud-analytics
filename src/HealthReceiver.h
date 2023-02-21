//
// Created by stephane bourque on 2022-03-15.
//

#pragma once

#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "framework/SubSystemServer.h"

namespace OpenWifi {
	class HealthMessage : public Poco::Notification {
	  public:
		explicit HealthMessage(const std::string &Key, const std::string &Payload)
			: Key_(Key), Payload_(Payload) {}
		const std::string &Key() { return Key_; }
		const std::string &Payload() { return Payload_; }

	  private:
		std::string Key_;
		std::string Payload_;
	};

	class VenueWatcher;

	class HealthReceiver : public SubSystemServer, Poco::Runnable {
	  public:
		static auto instance() {
			static auto instance_ = new HealthReceiver;
			return instance_;
		}

		int Start() override;
		void Stop() override;
		void run() override;
		void HealthReceived(const std::string &Key, const std::string &Payload);
		void Register(const std::vector<uint64_t> &SerialNumber, VenueWatcher *VW);
		void DeRegister(VenueWatcher *VW);

	  private:
		// map of mac(as int), list of (id,func)
		std::map<VenueWatcher *, std::set<uint64_t>> Notifiers_;
		uint64_t HealthWatcherId_ = 0;
		Poco::NotificationQueue Queue_;
		Poco::Thread Worker_;
		std::atomic_bool Running_ = false;

		HealthReceiver() noexcept
			: SubSystemServer("HealthReceiver", "HEALTH-RECEIVER", "health.receiver") {}
	};

	inline auto HealthReceiver() { return HealthReceiver::instance(); }

} // namespace OpenWifi