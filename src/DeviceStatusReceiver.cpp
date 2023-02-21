//
// Created by stephane bourque on 2022-03-13.
//

#include "DeviceStatusReceiver.h"
#include "VenueWatcher.h"
#include "fmt/core.h"
#include "framework/KafkaManager.h"
#include "framework/KafkaTopics.h"

namespace OpenWifi {
	int DeviceStatusReceiver::Start() {
		Running_ = true;
		Types::TopicNotifyFunction F = [this](const std::string &Key, const std::string &Payload) {
			this->DeviceStatusReceived(Key, Payload);
		};
		DeviceStateWatcherId_ = KafkaManager()->RegisterTopicWatcher(KafkaTopics::CONNECTION, F);
		Worker_.start(*this);
		return 0;
	}

	void DeviceStatusReceiver::Stop() {
		Running_ = false;
		KafkaManager()->UnregisterTopicWatcher(KafkaTopics::CONNECTION, DeviceStateWatcherId_);
		Queue_.wakeUpAll();
		Worker_.join();
	}

	void DeviceStatusReceiver::run() {
		Utils::SetThreadName("dev-status");
		Poco::AutoPtr<Poco::Notification> Note(Queue_.waitDequeueNotification());
		while (Note && Running_) {
			auto Msg = dynamic_cast<DeviceStatusMessage *>(Note.get());
			if (Msg != nullptr) {
				try {
					nlohmann::json msg = nlohmann::json::parse(Msg->Payload());
					if (msg.contains(uCentralProtocol::PAYLOAD)) {
						auto payload = msg[uCentralProtocol::PAYLOAD];

						uint64_t SerialNumber = Utils::SerialNumberToInt(Msg->Key());
						std::lock_guard G(Mutex_);

						for (const auto &[venue, devices] : Notifiers_) {
							if (devices.find(SerialNumber) != cend(devices)) {
								auto connection_data = std::make_shared<nlohmann::json>(payload);
								venue->PostConnection(SerialNumber, connection_data);
								break;
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

	void DeviceStatusReceiver::Register(const std::vector<uint64_t> &SerialNumber,
										VenueWatcher *VW) {
		std::lock_guard G(Mutex_);

		std::set<uint64_t> NewSerialNumbers;
		std::copy(SerialNumber.begin(), SerialNumber.end(),
				  std::inserter(NewSerialNumbers, NewSerialNumbers.begin()));
		auto it = Notifiers_.find(VW);
		if (it == end(Notifiers_))
			Notifiers_[VW] = NewSerialNumbers;
		else
			it->second = NewSerialNumbers;
	}

	void DeviceStatusReceiver::DeRegister(VenueWatcher *VW) {
		std::lock_guard G(Mutex_);
		Notifiers_.erase(VW);
	}

	void DeviceStatusReceiver::DeviceStatusReceived(const std::string &Key,
													const std::string &Payload) {
		std::lock_guard G(Mutex_);
		poco_debug(Logger(), fmt::format("Device({}): Connection/Ping message.", Key));
		Queue_.enqueueNotification(new DeviceStatusMessage(Key, Payload));
	}
} // namespace OpenWifi