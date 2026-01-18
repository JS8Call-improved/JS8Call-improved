/**
 * @brief This is a humble helper class for Qt signals and slots when this
 * mechanism is also used to initialize the data.  In that case, we first do the
 * "plumbing" phase, where signals and slots are connected as appropriate. After
 * that has been completed, signals are to be sent in a volley so that an
 * required initial values are being set.
 */

#include "TwoPhaseSignal.h"

TwoPhaseSignal::TwoPhaseSignal() {}

TwoPhaseSignal::~TwoPhaseSignal() {}
