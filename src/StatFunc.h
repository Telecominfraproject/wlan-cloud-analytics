//
// Created by stephane bourque on 2022-03-16.
//

#pragma once

#include <string>

namespace OpenWifi {

    class RelativeCounter {
    public:
        explicit RelativeCounter(uint64_t iv) {
            inited = iv;
            LastValue = iv;
        }

        RelativeCounter() = default;

        inline uint64_t set(uint64_t v) {
            if(!inited) {
                inited=true;
                LastValue=v;
                return v;
            }

            if(v>=LastValue) {
                Value = (v-LastValue);
                LastValue = v;
                return Value;
            } else {
                Value = v ;
                LastValue = v;
                return Value;
            }
        }

        [[nodiscard]] inline uint64_t get() const {
            return Value;
        }

    private:
        uint64_t Value = 0 ;
        bool inited = false;
        uint64_t LastValue=0;
    };

}