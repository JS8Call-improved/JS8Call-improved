#include "CallsignValidator.h"

CallsignValidator::CallsignValidator(QObject *parent, bool allow_compound)
    : QValidator{parent},
      re_{allow_compound ? R"(^[A-Za-z0-9/]+$)" : R"(^[A-Za-z0-9]+$)"} {}

auto CallsignValidator::validate(QString &input, int &pos) const -> State {
    auto match =
        re_.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
    input = input.toUpper();
    if (input.count(QLatin1Char('/')) > 2)
        return Invalid;
    if (match.hasMatch())
        return Acceptable;
    if (!input.size() || match.hasPartialMatch())
        return Intermediate;
    pos = input.size();
    return Invalid;
}
