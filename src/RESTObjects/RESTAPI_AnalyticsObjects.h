//
// Created by stephane bourque on 2022-01-10.
//

#pragma once
#include "framework/MicroService.h"

namespace OpenWifi {

    namespace AnalyticsObjects {

        struct Report {
            uint64_t            snapShot=0;
            Types::CountedMap   tenants;

            void        reset();
            void to_json(Poco::JSON::Object &Obj) const;
        };

    }

}