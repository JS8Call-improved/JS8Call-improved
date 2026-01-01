#ifndef PROCESSTHREAD_H
#define PROCESSTHREAD_H

/**
 * (C) 2019 Jordan Sherer <kn4crd@gmail.com> - All Rights Reserved
 **/

#include <QProcess>
#include <QScopedPointer>
#include <QThread>

class ProcessThread : public QThread {
    Q_OBJECT
  public:
    ProcessThread(QObject *parent = nullptr);
    ~ProcessThread();

    void setProcess(QProcess *proc, int msecs = 1000);
    QProcess *process() { return m_proc.data(); }

  protected:
    QScopedPointer<QProcess> m_proc;
};

#endif // PROCESSTHREAD_H
