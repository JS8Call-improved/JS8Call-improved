#ifndef TRACE_FILE_HPP_
#define TRACE_FILE_HPP_

#include "JS8_Include/pimpl_h.h"

class QString;

class TraceFile final {
  public:
    explicit TraceFile(QString const &TraceFile_file_path);
    ~TraceFile();

    // copying not allowed
    TraceFile(TraceFile const &) = delete;
    TraceFile &operator=(TraceFile const &) = delete;

  private:
    class impl;
    pimpl<impl> m_;
};

#endif
