//
// Created by stephane bourque on 2022-03-11.
//

#include "APStats.h"

namespace OpenWifi {

    void AP::Update(std::shared_ptr<nlohmann::json> &State) {
        std::cout << "MAC: " << Utils::IntToSerialNumber(mac_) << std::endl;
    }

}