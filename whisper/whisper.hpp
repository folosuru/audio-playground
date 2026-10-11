#ifndef INCLUDE_WHISPER_WHISPER_HPP_
#define INCLUDE_WHISPER_WHISPER_HPP_

#include "Utils.hpp"
#include <cstddef>
#include <istream>
#include <ostream>

struct WorldParams {
  double frame_period;
  int fs;
  int f0_length;
  int fft_size;
  std::unique_ptr<double[]> f0;
  std::unique_ptr<double[]> time_axis;

  CStyle2DArrayCompat<double> spectrogram;
  CStyle2DArrayCompat<double> aperiodicity;

  static WorldParams read(std::istream &, bool &success);
  void write(std::ostream &);
  double index_to_freq(size_t i) const {
    return static_cast<double>(i) * fs * fft_size;
  }
};

CStyle2DArrayCompat<double> PhantomShilhouette(WorldParams &param);

#endif // INCLUDE_WHISPER_WHISPER_HPP_
