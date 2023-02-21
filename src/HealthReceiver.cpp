//
// Created by stephane bourque on 2022-03-15.
//

#include "HealthReceiver.h"
#include "VenueWatcher.h"
#include "fmt/core.h"
#include "framework/KafkaManager.h"
#include "framework/KafkaTopics.h"

namespace OpenWifi {
	int HealthReceiver::Start() {
		Running_ = true;
		Types::TopicNotifyFunction F = [this](const std::string &Key, const std::string &Payload) {
			this->HealthReceived(Key, Payload);
		};
		HealthWatcherId_ = KafkaManager()->RegisterTopicWatcher(KafkaTopics::HEALTHCHECK, F);
		Worker_.start(*this);
		return 0;
	}

	void HealthReceiver::Stop() {
		Running_ = false;
		KafkaManager()->UnregisterTopicWatcher(KafkaTopics::HEALTHCHECK, HealthWatcherId_);
		Queue_.wakeUpAll();
		Worker_.join();
	}

	void HealthReceiver::run() {
		Utils::SetThreadName("dev-health");
		Poco::AutoPtr<Poco::Notification> Note(Queue_.waitDequeueNotification());
		while (Note && Running_) {
			auto Msg = dynamic_cast<HealthMessage *>(Note.get());
			if (Msg != nullptr) {
				try {
					nlohmann::json msg = nlohmann::json::parse(Msg->Payload());
					if (msg.contains(uCentralProtocol::PAYLOAD)) {
						auto payload = msg[uCentralProtocol::PAYLOAD];

						uint64_t SerialNumber = Utils::SerialNumberToInt(Msg->Key());
						std::lock_guard G(Mutex_);

						for (const auto &[venue, devices] : Notifiers_) {
							if (devices.find(SerialNumber) != cend(devices)) {
								auto health_data = std::make_shared<nlohmann::json>(payload);
								venue->PostHealth(SerialNumber, health_data);
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

	void HealthReceiver::Register(const std::vector<uint64_t> &SerialNumber, VenueWatcher *VW) {
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

	void HealthReceiver::DeRegister(VenueWatcher *VW) {
		std::lock_guard G(Mutex_);
		Notifiers_.erase(VW);
	}

	void HealthReceiver::HealthReceived(const std::string &Key, const std::string &Payload) {
		std::lock_guard G(Mutex_);
		poco_debug(Logger(), fmt::format("Device({}): Health message.", Key));
		Queue_.enqueueNotification(new HealthMessage(Key, Payload));
	}
} // namespace OpenWifi