//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

namespace OpenWifi {

    void AP::UpdateStats(std::shared_ptr<nlohmann::json> &State) {
        std::cout << "Stats update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
        std::cout << *State << std::endl;
    }

    void AP::UpdateConnection(std::shared_ptr<nlohmann::json> &Connection) {
        std::cout << "Connection update for MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
        std::cout << *Connection << std::endl;
    }

}