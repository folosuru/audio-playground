#ifndef INCLUDE_WHISPER_WHISPER_HPP_
#define INCLUDE_WHISPER_WHISPER_HPP_

#include "Utils.hpp"

struct WorldParams {
  double frame_period;
  int fs;

  std::unique_ptr<double[]> f0;
  std::unique_ptr<double[]> time_axis;
  int f0_length;

  CStyle2DArrCompat<double> spectrogram;
  CStyle2DArrCompat<double> aperiodicity;
  int fft_size;
};

CStyle2DArrCompat<double> PhantomShilhouette(WorldParams &param);

#endif // INCLUDE_WHISPER_WHISPER_HPP_
