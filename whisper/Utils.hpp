#ifndef INCLUDE_WHISPER_UTILS_HPP_
#define INCLUDE_WHISPER_UTILS_HPP_
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include <utility>

template <class T> class CStyle2DArrCompat {
public:
  struct element {
    T *ref;
    T *get() { return ref; }
    T &operator[](size_t index) { return ref[index]; }
    operator T *() const { return ref; }
  };

  CStyle2DArrCompat(int cnt) : raw_data(new T *[cnt]()), size(cnt) {}
  CStyle2DArrCompat() : raw_data(nullptr), size(0) {}
  CStyle2DArrCompat(const CStyle2DArrCompat &) = delete;
  CStyle2DArrCompat &operator=(const CStyle2DArrCompat &) = delete;

  CStyle2DArrCompat(CStyle2DArrCompat &&other) noexcept
      : raw_data(other.raw_data), size(other.size) {
    other.raw_data = nullptr;
    other.size = 0;
  }

  CStyle2DArrCompat &operator=(CStyle2DArrCompat &&other) noexcept {
    if (this != &other) {
      reset();
      raw_data = other.raw_data;
      size = other.size;
      other.raw_data = nullptr;
      other.size = 0;
    }
    return *this;
  }

  ~CStyle2DArrCompat() { reset(); }

  void reset() {
    if (raw_data == nullptr)
      return;

    for (int i = 0; i < size; i++) {
      if (raw_data[i] == nullptr)
        continue;

      delete[] raw_data[i];
    }
    delete[] raw_data;
    raw_data = nullptr;
    size = 0;
  }

  T *serve_elem(int index, int cnt) {
    if (size <= index)
      return nullptr;

    delete[] raw_data[index];
    raw_data[index] = new T[cnt];
    return raw_data[index];
  }

  void serve_all(int array_count, int array_size) {
    serve(array_count);
    for (int i = 0; i < array_count; i++) {
      serve_elem(i, array_size);
    }
  }

  T **serve(int cnt) {
    delete[] raw_data;
    raw_data = new T *[cnt]();
    size = cnt;
    return raw_data;
  }

  T **get() { return raw_data; }

  element operator[](size_t index) { return {raw_data[index]}; }

private:
  T **raw_data = nullptr;
  int size = 0;
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
