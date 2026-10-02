#include "Utils.hpp"
#include "whisper.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <numbers>
#include <print>

static double GetF0Median(WorldParams &param) {
  std::unique_ptr<double[]> tmp = std::make_unique<double[]>(param.f0_length);
  for (int i = 0; i < param.f0_length; i++) {
    tmp[i] = param.f0[i];
  }
  std::sort(tmp.get(), tmp.get() + param.f0_length);
  return tmp[param.f0_length / 2];
}

void LowFreqSuppression(WorldParams &param, double f0_median,
                        CStyle2DArrCompat<double> &result) {
  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      double bin_freq = static_cast<double>(j) * param.fs / param.fft_size;
      if (bin_freq < 550) {
        result[i][j] *= 0.2;
        continue;
      }
      if (1350 < bin_freq) {
        continue;
      }

      double weight_base = (bin_freq - 550) / (1350 - 550);
      result[i][j] *= pow(weight_base, std::numbers::e);
    }
  }
}

CStyle2DArrCompat<double> PhantomShilhouette(WorldParams &param) {
  CStyle2DArrCompat<double> result;
  result.serve_all(param.f0_length, param.fft_size / 2 + 1);

  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      result[i][j] = param.spectrogram[i][j];
    }
  }

  double F0_median = GetF0Median(param);
  std::println("0");
  LowFreqSuppression(param, F0_median, result);
  return result;
}
