#include "Utils.hpp"
#include "whisper.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>
#include <numbers>
#include <print>

static double GetF0Median(WorldParams &param) {
  std::unique_ptr<double[]> tmp = std::make_unique<double[]>(param.f0_length);
  for (int i = 0; i < param.f0_length; i++) {
    tmp[i] = param.f0[i];
  }
  std::sort(tmp.get(), tmp.get() + param.f0_length);
  int not_0_start = 0;
  for (; not_0_start < param.f0_length; not_0_start++) {
    if (tmp[not_0_start] <= std::numeric_limits<double>::epsilon()) {
      continue;
    }
    break;
  }
  const int midpoint = ((param.f0_length - not_0_start) / 2) + not_0_start;
  std::println(stderr, "F0 median: {}", tmp[midpoint]);
  return tmp[midpoint];
}

static constexpr double erb(double f) { return 9.2645 * log(1 + f / 228.83); }

static double reverse_erb(double E) { return 228.83 * (exp(E / 9.2645) - 1); }

// return before freq by after
static double F1_F2_Shift_transform(double after, double f0_median) {
  if (1600 <= after) {
    return after;
  }
  if (1100 < after) {
    const double start_x = erb(1100);
    const double end_x = erb(1600);
    const double start_y = erb(1000);
    const double end_y = erb(1600);

    const double input_erb = erb(after);
    const double progress = ((input_erb - start_x) / (end_x - start_x));

    return reverse_erb(std::lerp(start_y, end_y, progress));
  }
  const double start_x =
      erb(std::lerp(600, 400, ((f0_median - 80) / (260 - 80))));
  const double end_x = erb(1100);
  const double start_y = erb(400);
  const double end_y = erb(1000);

  const double input_erb = erb(after);
  const double progress = ((input_erb - start_x) / (end_x - start_x));

  return reverse_erb(std::lerp(start_y, end_y, progress));
}

static void F1_F2_Shift(WorldParams &param, double f0_median,
                        CStyle2DArrayCompat<double> &result) {
  for (int i = 0; i < param.f0_length; ++i) {
    auto result_array = result[i];
    auto from_array = param.spectrogram[i];

    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      double target_bin_freq =
          static_cast<double>(j) * param.fs / param.fft_size;

      if (1600 < target_bin_freq) {
        break;
      }
      double shift_from = F1_F2_Shift_transform(target_bin_freq, f0_median);
      int shift_index =
          static_cast<int>(shift_from * param.fft_size / param.fs);
      if (shift_index < 0 || (param.fft_size / 2 + 1) <= shift_index) {
        continue;
      }
      result_array[j] = from_array[shift_index];
    }
  }
}

static void HighFreqCompensation(WorldParams &param, double f0_median,
                                 CStyle2DArrayCompat<double> &result) {
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
                               CStyle2DArrayCompat<double> &result) {
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

CStyle2DArrayCompat<double> PhantomShilhouette(WorldParams &param) {
  CStyle2DArrayCompat<double> result;
  result.serve(param.f0_length, param.fft_size / 2 + 1);

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
