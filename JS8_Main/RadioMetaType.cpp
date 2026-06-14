/**
 * @file RadioMetaType.cpp
 * @brief extension of the Radio namespace
 */

#include "Radio.h"

#include <QDataStream>
#include <QDebug>
#include <QMetaType>

namespace Radio {
void register_types() {
    qRegisterMetaType<Radio::Frequency>("Frequency");
    qRegisterMetaType<Radio::FrequencyDelta>("FrequencyDelta");
    qRegisterMetaType<Radio::Frequencies>("Frequencies");
}
} // namespace Radio
