/**
 * @file DriftingDateTime.cpp
 * @brief Implementation of DriftingDateTimeSingleton
 */
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QThread>

#include "DriftingDateTime.h"

Q_DECLARE_LOGGING_CATEGORY(driftingdatetime_js8)

/**
 * @brief Get the singleton instance
 * 
 * @return DriftingDateTimeSingleton& 
 */
DriftingDateTimeSingleton &DriftingDateTimeSingleton::getSingleton() {
    if (singleton.isNull()) {
        singleton = QPointer{new DriftingDateTimeSingleton{}};
    }
    return *(singleton.data());
}

/**
 * @brief Construct a new Drifting Date Time Singleton:: Drifting Date Time Singleton object
 * 
 */
DriftingDateTimeSingleton::DriftingDateTimeSingleton() : driftMS(0) {}

/**
 * @brief Retrieve drift, in milliseconds.
 *
 * Positive values indicate the drifted clock is behind the system clock,
 * negative, it is early.
 * 
 * @return qint64 
 */
qint64 DriftingDateTimeSingleton::drift() const {
    QMutexLocker locker(&mutex);
    return driftMS;
}

/**
 * @brief Set the drift inner
 * 
 * @param ms 
 */
void DriftingDateTimeSingleton::setDriftInner(qint64 ms) {
    QMutexLocker locker(&mutex);
    driftMS = ms;
}

/**
 * @brief Set the drift
 * 
 * @param ms 
 */
void DriftingDateTimeSingleton::setDrift(qint64 ms) {
    qint64 old_drift = drift();
    setDriftInner(ms);
    if (ms != old_drift) {
        qCDebug(driftingdatetime_js8)
            << "Changed drift from" << old_drift << "to" << ms << "ms";
        emit driftChanged(ms);
    } else {
        qCDebug(driftingdatetime_js8)
            << "Incoming signal without change of drift, still" << old_drift
            << "ms";
    }
}

/**
 * @brief Emits to the driftChanged signal (as per TwoPhaseSignal contract).
 * 
 */
void DriftingDateTimeSingleton::onPlumbingCompleted() const {
    emit driftChanged(drift());
}

/**
 * @brief Static member initialization
 * 
 */
QPointer<DriftingDateTimeSingleton> DriftingDateTimeSingleton::singleton =
    QPointer<DriftingDateTimeSingleton>{};

Q_LOGGING_CATEGORY(driftingdatetime_js8, "driftingdatetime.js8", QtWarningMsg)
