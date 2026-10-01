#include "external/World/tools/audioio.h"
#include "synthesizer.hpp"
#include "world/constantnumbers.h"
#include "world/dio.h"

#include "Utils.hpp"
#include "world/stonemask.h"

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

  int out_len;
  auto wave = freq_to_wave(param.f0.get(), param.f0_length, out_len);

  wavwrite(wave.get(), out_len, 44100, 16, "out.wav");
}
