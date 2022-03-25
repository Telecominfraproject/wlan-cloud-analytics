//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include <boost/algorithm/string.hpp>
#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Environment.h"

#include "Daemon.h"
#include "StorageService.h"
#include "VenueCoordinator.h"
#include "StateReceiver.h"
#include "DeviceStatusReceiver.h"
#include "HealthReceiver.h"

namespace OpenWifi {
	class Daemon *Daemon::instance_ = nullptr;

	class Daemon *Daemon::instance() {
		if (instance_ == nullptr) {
			instance_ = new Daemon(vDAEMON_PROPERTIES_FILENAME,
								   vDAEMON_ROOT_ENV_VAR,
								   vDAEMON_CONFIG_ENV_VAR,
								   vDAEMON_APP_NAME,
								   vDAEMON_BUS_TIMER,
								   SubSystemVec{
									   OpenWifi::StorageService(),
                                       StateReceiver(),
                                       DeviceStatusReceiver(),
                                       HealthReceiver(),
                                       VenueCoordinator()
								   });
		}
		return instance_;
	}

    void Daemon::initialize([[maybe_unused]] Poco::Util::Application &self) {
        std::cout << "Daemon initializing" << std::endl;
        MicroService::instance().initialize(self);
        std::cout << "Daemon done" << std::endl;
    }
}

int main(int argc, char **argv) {
	try {
		auto App = OpenWifi::Daemon::instance();
		auto ExitCode =  App->run(argc, argv);
		delete App;

		return ExitCode;

	} catch (Poco::Exception &exc) {
		std::cerr << exc.displayText() << std::endl;
		return Poco::Util::Application::EXIT_SOFTWARE;
	}
}

// end of namespace