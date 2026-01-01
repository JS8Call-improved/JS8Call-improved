/**
 * This file is part of JS8Call.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * (C) 2019 Jordan Sherer <kn4crd@gmail.com> - All Rights Reserved
 *
 **/
#include "ProcessThread.h"

ProcessThread::ProcessThread(QObject *parent) : QThread(parent) {}

ProcessThread::~ProcessThread() { setProcess(nullptr); }

/**
 * @brief ProcessThread::setProcess
 * @param proc - process to move to this thread and take ownership
 */
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
