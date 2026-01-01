#include "FrequencyLineEdit.h"

#include <limits>

#include <QDoubleValidator>
#include <QLocale>
#include <QString>

#include "moc_FrequencyLineEdit.cpp"

namespace {
class MHzValidator : public QDoubleValidator {
  public:
    MHzValidator(double bottom, double top, QObject *parent = nullptr)
        : QDoubleValidator{bottom, top, 6, parent} {}

    State validate(QString &input, int &pos) const override {
        State result = QDoubleValidator::validate(input, pos);
        if (Acceptable == result) {
            bool ok;
            (void)QLocale{}.toDouble(input, &ok);
            if (!ok) {
                result = Intermediate;
            }
        }
        return result;
    }
};
} // namespace

FrequencyLineEdit::FrequencyLineEdit(QWidget *parent) : QLineEdit(parent) {
    setValidator(new MHzValidator{
        0., std::numeric_limits<Radio::Frequency>::max() / 10.e6, this});
}

auto FrequencyLineEdit::frequency() const -> Frequency {
    return Radio::frequency(text(), 6);
}

void FrequencyLineEdit::frequency(Frequency f) {
    setText(Radio::frequency_MHz_string(f));
}

FrequencyDeltaLineEdit::FrequencyDeltaLineEdit(QWidget *parent)
    : QLineEdit(parent) {
    setValidator(new MHzValidator{
        -std::numeric_limits<FrequencyDelta>::max() / 10.e6,
        std::numeric_limits<FrequencyDelta>::max() / 10.e6, this});
}

auto FrequencyDeltaLineEdit::frequency_delta() const -> FrequencyDelta {
    return Radio::frequency_delta(text(), 6);
}

void FrequencyDeltaLineEdit::frequency_delta(FrequencyDelta d) {
    setText(Radio::frequency_MHz_string(d));
}
