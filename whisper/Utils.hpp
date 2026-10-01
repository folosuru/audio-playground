#ifndef INCLUDE_WHISPER_UTILS_HPP_
#define INCLUDE_WHISPER_UTILS_HPP_
#include <chrono>
#include <iomanip>
#include <ios>
#include <memory>
#include <ostream>
template <class T> class CStyle2DArrCompat {
public:
  CStyle2DArrCompat() : data(nullptr), raw_data(nullptr) {}

  CStyle2DArrCompat(int cnt)
      : data(std::make_unique<std::unique_ptr<T>[]>(cnt)),
        raw_data(data.get()) {}

  auto &get_unique_ptr() { return data; }

  T **get() { return raw_data; }

private:
  std::unique_ptr<std::unique_ptr<T[]>[]> data;

  T **raw_data;
};

class timer {
public:
  template <typename func_t> timer(func_t function) {
    using std::chrono::system_clock;
    start = system_clock::now();
    function();
    end = system_clock::now();
  }

  auto delta() const { return end - start; }

  double seconds() {
    using std::chrono::duration_cast;
    using std::chrono::nanoseconds;
    return duration_cast<nanoseconds>(delta()).count() / 1000000000.0;
  }

  std::chrono::time_point<std::chrono::system_clock> start, end;
};

inline std::ostream &operator<<(std::ostream &os, timer &t) {
  return os << std::fixed << std::setprecision(6) << t.seconds();
}

#endif // INCLUDE_WHISPER_UTILS_HPP_
