//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "Daemon.h"

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Environment.h"
#include "Poco/Net/SSLManager.h"
#include "framework/OpenWifiTypes.h"

#include "StorageService.h"
#include "VenueCoordinator.h"
#include "StateReceiver.h"
#include "DeviceStatusReceiver.h"
#include "HealthReceiver.h"
#include "WifiClientCache.h"

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
                                       VenueCoordinator(),
                                       WifiClientCache()
								   });
		}
		return instance_;
	}

    void Daemon::PostInitialization([[maybe_unused]] Poco::Util::Application &self) {
    }

    void DaemonPostInitialization(Poco::Util::Application &self) {
        Daemon()->PostInitialization(self);
    }
}

int main(int argc, char **argv) {
    int ExitCode;
    try {
        Poco::Net::SSLManager::instance().initializeServer(nullptr, nullptr, nullptr);
        auto App = OpenWifi::Daemon::instance();
        ExitCode =  App->run(argc, argv);
        Poco::Net::SSLManager::instance().shutdown();
    } catch (Poco::Exception &exc) {
        ExitCode = Poco::Util::Application::EXIT_SOFTWARE;
        std::cout << exc.displayText() << std::endl;
    } catch (std::exception &exc) {
        ExitCode = Poco::Util::Application::EXIT_TEMPFAIL;
        std::cout << exc.what() << std::endl;
    } catch (...) {
        ExitCode = Poco::Util::Application::EXIT_TEMPFAIL;
        std::cout << "Exception on closure" << std::endl;
    }

    std::cout << "Exitcode: " << ExitCode << std::endl;
    return ExitCode;
}

// end of namespace