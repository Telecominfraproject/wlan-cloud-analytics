//
// Created by stephane bourque on 2022-03-16.
//

#pragma once

#include <string>
#include <ostream>

namespace OpenWifi {

    class RelativeCounter {
    public:
        explicit RelativeCounter(uint64_t iv) {
            inited = true;
            LastValue = iv;
        }

        RelativeCounter() = default;

        RelativeCounter & operator=(uint64_t v) {
            set(v);
            return *this;
        }

        bool operator==(uint64_t v) const {
            return v == Value;
        }
        bool operator>=(uint64_t v) const {
            return v >= Value;
        }
        bool operator>(uint64_t v) const {
            return v > Value;
        }
        bool operator<=(uint64_t v) const {
            return v <= Value;
        }
        bool operator<(uint64_t v) const {
            return v < Value;
        }

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

        friend std::ostream& operator<<(std::ostream& os, const RelativeCounter& rc);

    private:
        uint64_t Value = 0 ;
        bool inited = false;
        uint64_t LastValue=0;
    };

    std::ostream & operator<<(std::ostream &os, const RelativeCounter &rc) {
        os << rc.get() ;
        return os;
    }

}