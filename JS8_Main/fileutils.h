#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QFile>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

void flushFileBuffer(const QFile &f);

#endif // FILEUTILS_H
