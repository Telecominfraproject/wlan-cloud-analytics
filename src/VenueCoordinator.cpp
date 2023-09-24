//
// Created by stephane bourque on 2022-03-10.
//

#include "VenueCoordinator.h"
#include "StorageService.h"
#include "VenueWatcher.h"
#include "fmt/core.h"
#include "framework/MicroServiceFuncs.h"
#include "sdks/SDK_prov.h"

namespace OpenWifi {

	int VenueCoordinator::Start() {
		poco_notice(Logger(), "Starting...");
		GetBoardList();
		Worker_.start(*this);

		ReconcileTimerCallback_ = std::make_unique<Poco::TimerCallback<VenueCoordinator>>(
			*this, &VenueCoordinator::onReconcileTimer);
		ReconcileTimerTimer_.setStartInterval(3 * 60 * 1000);
		ReconcileTimerTimer_.setPeriodicInterval(3 * 60 * 1000); // 1 hours
		ReconcileTimerTimer_.start(*ReconcileTimerCallback_, MicroServiceTimerPool());

		return 0;
	}

	void VenueCoordinator::onReconcileTimer([[maybe_unused]] Poco::Timer &timer) {
		std::lock_guard G(Mutex_);
		Utils::SetThreadName("brd-refresh");

		poco_information(Logger(), "Starting to reconcile board information.");
		for (const auto &[board_id, watcher] : Watchers_) {
			poco_information(Logger(), fmt::format("Updating: {}", board_id));
			UpdateBoard(board_id);
		}
		poco_information(Logger(), "Finished reconciling board information.");
	}

	void VenueCoordinator::GetBoardList() {
		BoardsToWatch_.clear();
		auto F = [&](const AnalyticsObjects::BoardInfo &B) -> bool {
			BoardsToWatch_.insert(B);
			// poco_information(Logger(),fmt::format("Starting watch for {}.", B.info.name));
			return true;
		};
		StorageService()->BoardsDB().Iterate(F);
	}

	void VenueCoordinator::Stop() {
		poco_notice(Logger(), "Stopping...");
		Running_ = false;
		Worker_.wakeUp();
		Worker_.wakeUp();
		Worker_.join();
		poco_notice(Logger(), "Stopped...");
	}

	void VenueCoordinator::run() {
		Utils::SetThreadName("venue-coord");
		Running_ = true;
		while (Running_) {
			Poco::Thread::trySleep(60000);
			if (!Running_)
				break;

			std::lock_guard G(Mutex_);
			GetBoardList();

			if (!BoardsToWatch_.empty()) {
				for (const auto &board_to_start : BoardsToWatch_) {
					bool VenueExists = true;
					if (!Watching(board_to_start.info.id)) {
						StartBoard(board_to_start);
					} else if (SDK::Prov::Venue::Exists(nullptr, board_to_start.venueList[0].id,
														VenueExists) &&
							   !VenueExists) {
						RetireBoard(board_to_start);
					}
				}
			}
		}
	}

	void VenueCoordinator::RetireBoard(const AnalyticsObjects::BoardInfo &B) {
		Logger().error(fmt::format(
			"Venue board '{}' is no longer in the system. Retiring its associated board.",
			B.venueList[0].name));
		StopBoard(B.info.id);
		StorageService()->BoardsDB().DeleteRecord("id", B.info.id);
		StorageService()->TimePointsDB().DeleteRecords(fmt::format(" boardId='{}' ", B.info.id));
	}

	bool VenueCoordinator::GetDevicesForBoard(const AnalyticsObjects::BoardInfo &B,
											  std::vector<uint64_t> &Devices, bool &VenueExists) {
		ProvObjects::VenueDeviceList VDL;

		if (SDK::Prov::Venue::GetDevices(nullptr, B.venueList[0].id,
										 B.venueList[0].monitorSubVenues, VDL, VenueExists)) {
			Devices.clear();
			for (const auto &device : VDL.devices) {
				Devices.push_back(Utils::SerialNumberToInt(device));
			}
			std::sort(Devices.begin(), Devices.end());
			auto LastDevice = std::unique(Devices.begin(), Devices.end());
			Devices.erase(LastDevice, Devices.end());
			return true;
		}

		if (!VenueExists) {
			RetireBoard(B);
		}

		return false;
	}

	bool VenueCoordinator::StartBoard(const AnalyticsObjects::BoardInfo &B) {
		if (B.venueList.empty())
			return true;

		bool VenueExists = true;
		std::vector<uint64_t> Devices;
		if (GetDevicesForBoard(B, Devices, VenueExists)) {
			std::lock_guard G(Mutex_);
			ExistingBoards_[B.info.id] = Devices;
			Watchers_[B.info.id] =
				std::make_shared<VenueWatcher>(B.info.id, B.venueList[0].id, Logger(), Devices);
			Watchers_[B.info.id]->Start();
			poco_information(Logger(), fmt::format("Started board {} for venue {}", B.info.name,
												   B.venueList[0].id));
			return true;
		}

		if (!VenueExists) {
			RetireBoard(B);
			return false;
		}

		poco_information(Logger(), fmt::format("Could not start board {}", B.info.name));
		return false;
	}

	void VenueCoordinator::StopBoard(const std::string &id) {
		std::lock_guard G(Mutex_);

		auto it = Watchers_.find(id);
		if (it != Watchers_.end()) {
			it->second->Stop();
			Watchers_.erase(it);
		}
	}

	void VenueCoordinator::UpdateBoard(const std::string &id) {
		AnalyticsObjects::BoardInfo B;
		if (StorageService()->BoardsDB().GetRecord("id", id, B)) {
			std::vector<uint64_t> Devices;
			bool VenueExists = true;
			if (GetDevicesForBoard(B, Devices, VenueExists)) {
				std::lock_guard G(Mutex_);
				auto it = ExistingBoards_.find(id);
				if (it != ExistingBoards_.end()) {
					if (it->second != Devices) {
						auto it2 = Watchers_.find(id);
						if (it2 != Watchers_.end()) {
							it2->second->ModifySerialNumbers(Devices);
						}
						ExistingBoards_[id] = Devices;
						poco_information(Logger(), fmt::format("Modified board {}", B.info.name));
					} else {
						poco_information(Logger(),
										 fmt::format("No device changes in board {}", B.info.name));
					}
				}
				return;
			}

			if (!VenueExists) {
				RetireBoard(B);
				return;
			}

			poco_information(Logger(), fmt::format("Could not modify board {}", B.info.name));
		}
	}

	bool VenueCoordinator::Watching(const std::string &id) {
		std::lock_guard G(Mutex_);
		return (ExistingBoards_.find(id) != ExistingBoards_.end());
	}

	void VenueCoordinator::AddBoard(const std::string &id) {
		std::lock_guard G(Mutex_);

		AnalyticsObjects::BoardInfo B;
		if (StorageService()->BoardsDB().GetRecord("id", id, B))
			BoardsToWatch_.insert(B);
		else
			poco_information(Logger(), fmt::format("Board {} does not seem to exist", id));
	}

	void VenueCoordinator::GetDevices(std::string &id, AnalyticsObjects::DeviceInfoList &DIL) {
		std::lock_guard G(Mutex_);

		auto it = Watchers_.find(id);
		if (it != end(Watchers_)) {
			it->second->GetDevices(DIL.devices);
		}
	}
} // namespace OpenWifi