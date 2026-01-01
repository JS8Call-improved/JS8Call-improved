#ifndef JSC_CHECKER_H
#define JSC_CHECKER_H

/**
 * (C) 2018 Jordan Sherer <kn4crd@gmail.com> - All Rights Reserved
 **/

#include <QObject>

class QTextEdit;

class JSCChecker : public QObject
{
    Q_OBJECT
public:
    explicit JSCChecker(QObject *parent = nullptr);

    static void checkRange(QTextEdit * edit, int start, int end);
    static QStringList suggestions(QString word, int n, bool *pFound);

signals:

public slots:

private:
};

#endif // JSC_CHECKER_H
