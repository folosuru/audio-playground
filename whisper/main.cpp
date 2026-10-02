#include "external/World/tools/audioio.h"
#include "synthesizer.hpp"
#include "world/cheaptrick.h"
#include "world/constantnumbers.h"
#include "world/d4c.h"
#include "world/dio.h"

#include "Utils.hpp"
#include "world/stonemask.h"
#include "world/synthesis.h"

#include <cstdlib>
#include <memory>
#include <print>

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

void estimate_F0(double *x, int len, WorldParams &param) {
  DioOption option;
  InitializeDioOption(&option);
  option.frame_period = param.frame_period;
  option.speed = 1;
  option.f0_floor = world::kFloorF0;
  option.allowed_range = 0.1;
  param.f0_length = GetSamplesForDIO(param.fs, len, param.frame_period);
  param.f0 = std::make_unique<double[]>(param.f0_length);
  param.time_axis = std::make_unique<double[]>(param.f0_length);

  std::unique_ptr<double[]> tmp_f0 =
      std::make_unique<double[]>(param.f0_length);

  auto dio_erappse = timer([&]() {
    Dio(x, len, param.fs, &option, param.time_axis.get(), tmp_f0.get());
  });
  std::println(stderr, "Dio() take {} sec", dio_erappse.seconds());

  auto stonemase_erappse = timer([&]() {
    StoneMask(x, len, param.fs, param.time_axis.get(), tmp_f0.get(),
              param.f0_length, param.f0.get());
  });
  std::println(stderr, "StoneMask() take {} sec", stonemase_erappse.seconds());
}

void estimate_spectral_envelope(double *x, int len, WorldParams &params) {
  CheapTrickOption option;
  InitializeCheapTrickOption(params.fs, &option);
  option.q1 = -0.15;
  option.f0_floor = 71.0;

  params.fft_size = GetFFTSizeForCheapTrick(params.fs, &option);
  params.spectrogram.serve(params.f0_length);
  for (int i = 0; i < params.f0_length; ++i) {
    params.spectrogram.serve_elem(i, params.fft_size / 2 + 1);
  }
  std::println(stderr, "a");

  auto take = timer([&]() {
    CheapTrick(x, len, params.fs, params.time_axis.get(), params.f0.get(),
               params.f0_length, &option, params.spectrogram.get());
  });

  std::println(stderr, "CheapTrick() take {} sec", take.seconds());
}

void estimate_aperiodicity(double *x, int len, WorldParams &param) {
  D4COption option;
  InitializeD4COption(&option);

  param.aperiodicity.serve(param.f0_length);
  for (int i = 0; i < param.f0_length; i++) {
    param.aperiodicity.serve_elem(i, param.fft_size / 2 + 1);
  }
  auto d4c_take = timer([&]() {
    D4C(x, len, param.fs, param.time_axis.get(), param.f0.get(),
        param.f0_length, param.fft_size, &option, param.aperiodicity.get());
  });

  std::println(stderr, "D4C() take {} sec", d4c_take.seconds());
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::println(stderr, "usage: {} [filename]", argv[0]);
    exit(1);
  }
  const char *input_file = argv[1];

  int len = GetAudioLength(input_file);
  if (len <= 0) {
    std::println(stderr, "GetAudioLength returns {}.", len);
    exit(1);
  }

  std::unique_ptr<double[]> x = std::make_unique<double[]>(len);

  int fs, nbit;
  wavread(input_file, &fs, &nbit, x.get());
  std::print(stderr,
             "File Info:\n"
             "{} Hz {} bit wav\n"
             "{} samples\n{} sec\n",
             fs, nbit, len, static_cast<double>(len) / fs);

  WorldParams param;
  param.fs = fs;
  param.frame_period = 5.0;

  estimate_F0(x.get(), len, param);
  estimate_spectral_envelope(x.get(), len, param);
  estimate_aperiodicity(x.get(), len, param);

  std::unique_ptr<double[]> zero_filled_f0 =
      std::make_unique<double[]>(param.f0_length);
  for (int i = 0; i < param.f0_length; i++) {
    zero_filled_f0[i] = 0;
  }

  int y_len = 1 + static_cast<int>((param.f0_length - 1) * param.frame_period /
                                   1000.0 * param.fs);
  std::unique_ptr<double[]> y = std::make_unique<double[]>(y_len);
  Synthesis(zero_filled_f0.get(), param.f0_length, param.spectrogram.get(),
            param.aperiodicity.get(), param.fft_size, param.frame_period,
            param.fs, y_len, y.get());
  wavwrite(y.get(), y_len, param.fs, 16, "out_white.wav");

  int out_len;
  auto wave = freq_to_wave(param.f0.get(), param.f0_length, out_len);

  wavwrite(wave.get(), out_len, 44100, 16, "out.wav");
}
