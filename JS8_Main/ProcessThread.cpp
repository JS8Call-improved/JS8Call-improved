/**
 * @file ProcessThread.cpp
 * @brief ProcessThread::setProcess
 * @param proc - process to move to this thread and take ownership
 * (C) 2019 Jordan Sherer <kn4crd@gmail.com> - All Rights Reserved
 */
#include "ProcessThread.h"

ProcessThread::ProcessThread(QObject *parent) : QThread(parent) {}

ProcessThread::~ProcessThread() { setProcess(nullptr); }

void ProcessThread::setProcess(QProcess *proc, int msecs) {
    if (!m_proc.isNull()) {
        bool b = m_proc->waitForFinished(msecs);
        if (!b)
            m_proc->close();
        m_proc.reset();
    }

    if (proc) {
        proc->moveToThread(this);
        m_proc.reset(proc);
    }
}
