#include "../external/World/tools/audioio.h"
#include "world/cheaptrick.h"
#include "world/constantnumbers.h"
#include "world/d4c.h"
#include "world/dio.h"

#include "../Utils.hpp"
#include "../whisper.hpp"

#include "world/stonemask.h"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <print>

namespace {
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

  ProgressTimer("Dio()", [&]() {
    Dio(x, len, param.fs, &option, param.time_axis.get(), tmp_f0.get());
  });

  ProgressTimer("StoneMask()", [&]() {
    StoneMask(x, len, param.fs, param.time_axis.get(), tmp_f0.get(),
              param.f0_length, param.f0.get());
  });
}

void estimate_spectral_envelope(double *x, int len, WorldParams &params) {
  CheapTrickOption option;
  InitializeCheapTrickOption(params.fs, &option);
  option.q1 = -0.15;
  option.f0_floor = 71.0;

  params.fft_size = GetFFTSizeForCheapTrick(params.fs, &option);
  params.spectrogram.serve(params.f0_length, params.fft_size / 2 + 1);

  ProgressTimer("CheapTrick()", [&]() {
    CheapTrick(x, len, params.fs, params.time_axis.get(), params.f0.get(),
               params.f0_length, &option, params.spectrogram.get());
  });
}

void estimate_aperiodicity(double *x, int len, WorldParams &param) {
  D4COption option;
  InitializeD4COption(&option);

  param.aperiodicity.serve(param.f0_length, param.fft_size / 2 + 1);
  ProgressTimer("D4C()", [&]() {
    D4C(x, len, param.fs, param.time_axis.get(), param.f0.get(),
        param.f0_length, param.fft_size, &option, param.aperiodicity.get());
  });
}
} // namespace

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::println(stderr, "usage: {} [filename] [output]", argv[0]);
    exit(1);
  }
  const char *input_file = argv[1];
  const char *output_file = argv[2];

  int len = GetAudioLength(input_file);
  if (len <= 0) {
    std::println(stderr, "GetAudioLength returns {}.", len);
    exit(1);
  }

  std::unique_ptr<double[]> x = std::make_unique<double[]>(len);
  int fs, nbit;
  wavread(input_file, &fs, &nbit, x.get());
  std::print(stderr,
             "File: {}\n"
             "info: {} Hz / {} bit wav / "
             "{} samples / {:.5f} sec\n",
             argv[1], fs, nbit, len, static_cast<double>(len) / fs);

  WorldParams param;
  param.fs = fs;
  param.frame_period = 5.0;
  estimate_F0(x.get(), len, param);
  estimate_spectral_envelope(x.get(), len, param);
  estimate_aperiodicity(x.get(), len, param);
  std::println(stderr, "done");

  std::ofstream output(output_file);
  param.write(output);
}
