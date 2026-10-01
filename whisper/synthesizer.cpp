#include "synthesizer.hpp"
#include <cmath>
#include <memory>
#include <numbers>

std::unique_ptr<double[]> freq_to_wave(double *freq, int len, int &out_len) {
  const double total_time = len * 0.005;
  const int samples = static_cast<int>(total_time * 44100);
  std::unique_ptr<double[]> result = std::make_unique<double[]>(samples + 1);
  out_len = samples;

  double phase = 0.0;
  for (int i = 0; i < samples; ++i) {
    double time = i / 44100.;
    int index = time / 0.005;
    if (len <= index) {
      index = len - 1;
    }
    auto current_freq = freq[index];
    result[i] = sin(phase) * 0.9;
    phase += 2 * std::numbers::pi * current_freq / 44100;

    if (2 * std::numbers::pi <= phase) {
      phase = fmod(phase, (2 * std::numbers::pi));
    }
  }
  return result;
}
