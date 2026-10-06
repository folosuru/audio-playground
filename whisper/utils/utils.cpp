#include "../Utils.hpp"
#include "../whisper.hpp"

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <memory>
#include <ostream>
#include <print>

inline std::ostream &operator<<(std::ostream &os, timer &t) {
  return os << std::fixed << std::setprecision(6) << t.seconds();
}

template <class T> void os_write(std::ostream &os, const T &data) {
  os.write(reinterpret_cast<const char *>(&data), sizeof(T));
}

template <class T> void os_write_arr(std::ostream &os, const T *data, int len) {
  os.write(reinterpret_cast<const char *>(data), len * sizeof(T));
}
template <class T> void is_read(std::istream &os, T &data) {
  os.read(reinterpret_cast<char *>(&data), sizeof(T));
}
template <class T> void is_read_arr(std::istream &os, T *data, int len) {
  os.read(reinterpret_cast<char *>(data), len * sizeof(T));
}

void WorldParams::write(std::ostream &os) {
  os_write(os, frame_period);
  os_write(os, fs);
  os_write(os, f0_length);
  os_write(os, fft_size);
  os_write_arr(os, f0.get(), f0_length);
  os_write_arr(os, time_axis.get(), f0_length);
  spectrogram.write(os);
  aperiodicity.write(os);
}

WorldParams WorldParams::read(std::istream &is, bool &success) {
  WorldParams result;
  is_read(is, result.frame_period);
  is_read(is, result.fs);
  is_read(is, result.f0_length);
  is_read(is, result.fft_size);
  result.f0 = std::make_unique<double[]>(result.f0_length);
  is_read_arr(is, result.f0.get(), result.f0_length);
  result.time_axis = std::make_unique<double[]>(result.f0_length);
  is_read_arr(is, result.time_axis.get(), result.f0_length);
  result.spectrogram = CStyle2DArrayCompat<double>::read(is);
  result.aperiodicity = CStyle2DArrayCompat<double>::read(is);

  auto d2_except = result.fft_size / 2 + 1;
  if (!is || d2_except <= 0 || result.frame_period <= 0 || result.fs <= 0 ||
      result.spectrogram.index_size() != result.f0_length ||
      result.spectrogram.array_size() != d2_except ||
      result.aperiodicity.index_size() != result.f0_length ||
      result.aperiodicity.array_size() != d2_except) {
    std::println(stderr, "fail to read: file broken?");
    success = false;
    return {};
  }
  success = true;
  return result;
}

void csv_out(const char *filename, const std::unique_ptr<double[]> &array,
             size_t len) {
  std::ofstream file(filename);

  for (int i = 0; i < len; ++i) {
    file << array[i] << ", ";
  }
}
