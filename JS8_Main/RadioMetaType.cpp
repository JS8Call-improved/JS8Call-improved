#include "Radio.h"

#include <QDataStream>
#include <QDebug>
#include <QMetaType>

namespace Radio {
void register_types() {
    qRegisterMetaType<Radio::Frequency>("Frequency");
    qRegisterMetaType<Radio::FrequencyDelta>("FrequencyDelta");
    qRegisterMetaType<Radio::Frequencies>("Frequencies");

    // This is required to preserve v1.5 "frequencies" setting for
    // backwards compatibility, without it the setting gets trashed
    // by later versions.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<Radio::Frequencies>("Frequencies");
#endif
}
} // namespace Radio
