#ifndef FLATTEN_HPP__
#define FLATTEN_HPP__

#include <memory>

// Functor by which to flatten (or not, by default) a spectrum; not
// reentrant, but serially reusable.

class Flatten {
  public:
    // Constructor
    explicit Flatten(bool = false);

    // Destructor
    ~Flatten();

    // Turn flattening on or off
    void operator()(bool value);

    // Process (or not) the supplied spectrum data
    void operator()(float *data, std::size_t size);

    // Return active / inactive flattening status
    explicit operator bool() const noexcept { return !!m_impl; }
    bool live() const noexcept { return !!m_impl; }

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif
