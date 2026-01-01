#ifndef CALLSIGN_VALIDATOR_HPP__
#define CALLSIGN_VALIDATOR_HPP__

#include <QRegularExpression>
#include <QValidator>

//
// CallsignValidator - QValidator implementation for callsigns
//
class CallsignValidator final : public QValidator {
  public:
    CallsignValidator(QObject *parent = nullptr, bool allow_compound = true);

    // QValidator implementation
    State validate(QString &input, int &pos) const override;

  private:
    QRegularExpression re_;
};

#endif
