#ifndef INCLUDE_WHISPER_UTILS_HPP_
#define INCLUDE_WHISPER_UTILS_HPP_
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iosfwd>
#include <memory>
#include <print>
#include <span>
#include <type_traits>

template <class T> class CStyle2DArrayCompat {
public:
  CStyle2DArrayCompat() = default;
  CStyle2DArrayCompat(size_t d1_, size_t d2_) : d1(d1_), d2(d2_) {
    serve(d1, d2);
  }
  CStyle2DArrayCompat(CStyle2DArrayCompat &&other) noexcept
      : d1(std::exchange(other.d1, 0)), d2(std::exchange(other.d2, 0)),
        data(std::move(other.data)), data_index(std::move(other.data_index)) {}

  CStyle2DArrayCompat &operator=(CStyle2DArrayCompat &&other) noexcept {
    if (this != &other) {
      d1 = std::exchange(other.d1, 0);
      d2 = std::exchange(other.d2, 0);
      data = std::move(other.data);
      data_index = std::move(other.data_index);
    }
    return *this;
  }

  void serve(size_t d1_, size_t d2_) {
    d1 = d1_;
    d2 = d2_;
    data = std::make_unique<T[]>(d1 * d2);
    set_data_index();
  }

  T *operator[](size_t index) noexcept { return &data[index * d2]; }
  T **get() { return data_index.get(); }

  size_t index_size() const { return d1; }
  size_t array_size() const { return d2; }
  size_t size() const { return d1 * d2; }

  std::span<T> get_array(size_t index) {
    return std::span<T>((*this)[index], d2);
  }

  void write(std::ostream &os) const {
    static_assert(std::is_trivially_copyable<T>::value);
    os.write(reinterpret_cast<const char *>(&d1), sizeof(d1));
    os.write(reinterpret_cast<const char *>(&d2), sizeof(d2));
    os.write(reinterpret_cast<const char *>(data.get()), sizeof(T) * d1 * d2);
  }
  static CStyle2DArrayCompat read(std::istream &is) {
    static_assert(std::is_trivially_copyable<T>::value);
    size_t d1, d2;
    is.read(reinterpret_cast<char *>(&d1), sizeof(d1));
    is.read(reinterpret_cast<char *>(&d2), sizeof(d2));
    std::unique_ptr<T[]> buf = std::make_unique<T[]>(d1 * d2);
    is.read(reinterpret_cast<char *>(buf.get()), sizeof(T) * d1 * d2);
    return CStyle2DArrayCompat(d1, d2, std::move(buf));
  }

private:
  CStyle2DArrayCompat(size_t d1_, size_t d2_, std::unique_ptr<T[]> data_)
      : d1(d1_), d2(d2_), data(std::move(data_)) {
    set_data_index();
  }
  void set_data_index() {
    data_index = std::make_unique<T *[]>(d1);
    for (size_t i = 0; i < d1; ++i) {
      data_index[i] = &data[i * d2];
    }
  }

  size_t d1 = 0, d2 = 0;
  std::unique_ptr<T[]> data;
  std::unique_ptr<T *[]> data_index;
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

class ProgressTimer {
public:
  template <class func_t> ProgressTimer(const char *title, func_t function) {
    std::print(stderr, "-> {}", title);
    std::fflush(stderr);
    auto time = timer(function);
    std::println(stderr, " ({} sec) ", time.seconds());
  }
};

std::ostream &operator<<(std::ostream &os, timer &t);

void csv_out(const char *filename, const std::unique_ptr<double[]> &array,
             size_t len);

#endif // INCLUDE_WHISPER_UTILS_HPP_
