#include "JS8MessageBox.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QPushButton>

#include "revision_utils.h"

JS8MessageBox::JS8MessageBox(QWidget *parent) : QMessageBox{parent} {
    setWindowTitle(program_title());
}

JS8MessageBox::JS8MessageBox(Icon icon, QString const &text,
                             StandardButtons buttons, QWidget *parent,
                             Qt::WindowFlags flags)
    : QMessageBox{icon,   QCoreApplication::applicationName(),
                  text,   buttons,
                  parent, flags} {}

void JS8MessageBox::about_message(QWidget *parent, QString const &text) {
    QMessageBox::about(parent, program_title(), text);
}

void JS8MessageBox::about_Qt_message(QWidget *parent) {
    QMessageBox::aboutQt(parent, program_title());
}

namespace {
QMessageBox::StandardButton
show_it(QWidget *parent, JS8MessageBox::Icon icon, QString const &text,
        QString const &informative, QString const &detail,
        JS8MessageBox::StandardButtons buttons,
        JS8MessageBox::StandardButton default_button) {
    JS8MessageBox mb{icon, text, JS8MessageBox::NoButton, parent};
    QDialogButtonBox *button_box = mb.findChild<QDialogButtonBox *>();
    Q_ASSERT(button_box);

    uint mask = JS8MessageBox::FirstButton;
    while (mask <= JS8MessageBox::LastButton) {
        uint sb = buttons & mask;
        mask <<= 1;
        if (!sb)
            continue;
        QPushButton *button =
            mb.addButton(static_cast<JS8MessageBox::StandardButton>(sb));
        // Choose the first accept role as the default
        if (mb.defaultButton())
            continue;
        if ((default_button == JS8MessageBox::NoButton &&
             button_box->buttonRole(button) == QDialogButtonBox::AcceptRole) ||
            (default_button != JS8MessageBox::NoButton &&
             sb == static_cast<uint>(default_button)))
            mb.setDefaultButton(button);
    }
    mb.setInformativeText(informative);
    mb.setDetailedText(detail);
    if (mb.exec() == -1)
        return JS8MessageBox::Cancel;
    return mb.standardButton(mb.clickedButton());
}
} // namespace

auto JS8MessageBox::information_message(QWidget *parent, QString const &text,
                                        QString const &informative,
                                        QString const &detail,
                                        StandardButtons buttons,
                                        StandardButton default_button)
    -> StandardButton {
    return show_it(parent, Information, text, informative, detail, buttons,
                   default_button);
}

auto JS8MessageBox::query_message(QWidget *parent, QString const &text,
                                  QString const &informative,
                                  QString const &detail,
                                  StandardButtons buttons,
                                  StandardButton default_button)
    -> StandardButton {
    return show_it(parent, Question, text, informative, detail, buttons,
                   default_button);
}

auto JS8MessageBox::warning_message(QWidget *parent, QString const &text,
                                    QString const &informative,
                                    QString const &detail,
                                    StandardButtons buttons,
                                    StandardButton default_button)
    -> StandardButton {
    return show_it(parent, Warning, text, informative, detail, buttons,
                   default_button);
}

auto JS8MessageBox::critical_message(QWidget *parent, QString const &text,
                                     QString const &informative,
                                     QString const &detail,
                                     StandardButtons buttons,
                                     StandardButton default_button)
    -> StandardButton {
    return show_it(parent, Critical, text, informative, detail, buttons,
                   default_button);
}
