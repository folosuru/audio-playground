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
  std::println(stderr, "median of f0: {}", tmp[param.f0_length / 2]);
  return tmp[param.f0_length / 2];
}

// return before freq by after
static double F1_F2_Shift_transform(double after) {
  if (1600 < after) {
    return after;
  }
}

static void F1_F2_Shift(WorldParams &param, double f0_median,
                        CStyle2DArrCompat<double> &result) {
  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      double target_bin_freq =
          static_cast<double>(j) * param.fs / param.fft_size;

      if (1600 < target_bin_freq) {
        break;
      }
    }
  }
}

static void HighFreqCompensation(WorldParams &param, double f0_median,
                                 CStyle2DArrCompat<double> &result) {
  const double compensation_max = (-0.0064 * f0_median + 2.75);
  std::println("compensation max: {}", compensation_max);
  if (compensation_max <= 1) {
    return;
  }

  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      double bin_freq = static_cast<double>(j) * param.fs / param.fft_size;
      if (bin_freq < 1600) {
        continue;
      }
      if (10000 <= bin_freq) {
        result[i][j] *= compensation_max;
        continue;
      }
      double mul =
          (((bin_freq - 1600) / (10000 - 1600)) * (compensation_max - 1)) + 1;

      result[i][j] *= mul;
    }
  }
}

static void LowFreqSuppression(WorldParams &param, double f0_median,
                               CStyle2DArrCompat<double> &result) {
  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      double bin_freq = static_cast<double>(j) * param.fs / param.fft_size;
      if (bin_freq < 550) {
        result[i][j] *= 0.001;
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
  F1_F2_Shift(param, F0_median, result);
  HighFreqCompensation(param, F0_median, result);
  LowFreqSuppression(param, F0_median, result);
  return result;
}
