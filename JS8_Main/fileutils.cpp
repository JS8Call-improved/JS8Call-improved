#include "fileutils.h"

void flushFileBuffer(const QFile &f) {
#ifdef Q_OS_WIN
    FlushFileBuffers(reinterpret_cast<HANDLE>(f.handle()));
#elif _POSIX_SYNCHRONIZED_IO > 0
    fdatasync(f.handle());
#else
    fsync(f.handle());
#endif
}
