//
// Created by stephane bourque on 2022-03-10.
//

#pragma once

#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "framework/SubSystemServer.h"

namespace OpenWifi {
	class DeviceStatusMessage : public Poco::Notification {
	  public:
		explicit DeviceStatusMessage(const std::string &Key, const std::string &Payload)
			: Key_(Key), Payload_(Payload) {}
		const std::string &Key() { return Key_; }
		const std::string &Payload() { return Payload_; }

	  private:
		std::string Key_;
		std::string Payload_;
	};

	class VenueWatcher;

	class DeviceStatusReceiver : public SubSystemServer, Poco::Runnable {
	  public:
		static auto instance() {
			static auto instance_ = new DeviceStatusReceiver;
			return instance_;
		}

		int Start() override;
		void Stop() override;
		void run() override;
		void DeviceStatusReceived(const std::string &Key, const std::string &Payload);
		void Register(const std::vector<uint64_t> &SerialNumber, VenueWatcher *VW);
		void DeRegister(VenueWatcher *VW);

	  private:
		// map of mac(as int), list of (id,func)
		std::map<VenueWatcher *, std::set<uint64_t>> Notifiers_;
		uint64_t DeviceStateWatcherId_ = 0;
		Poco::NotificationQueue Queue_;
		Poco::Thread Worker_;
		std::atomic_bool Running_ = false;

		DeviceStatusReceiver() noexcept
			: SubSystemServer("DeviceStatus", "DEV-STATUS-RECEIVER", "devicestatus.receiver") {}
	};

	inline auto DeviceStatusReceiver() { return DeviceStatusReceiver::instance(); }

} // namespace OpenWifi