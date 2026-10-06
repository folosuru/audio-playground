#include "external/World/tools/audioio.h"
#include "synthesizer.hpp"
#include "world/cheaptrick.h"
#include "world/constantnumbers.h"
#include "world/d4c.h"
#include "world/dio.h"

#include "Utils.hpp"
#include "whisper.hpp"

#include "world/stonemask.h"
#include "world/synthesis.h"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <print>

void output_with_whitenoize(const char *filename, WorldParams &param,
                            CStyle2DArrayCompat<double> &spectrogram) {
  std::unique_ptr<double[]> zero_filled_f0 =
      std::make_unique<double[]>(param.f0_length);
  for (int i = 0; i < param.f0_length; i++) {
    zero_filled_f0[i] = 0;
  }

  CStyle2DArrayCompat<double> one_filled_aperiodicity;
  one_filled_aperiodicity.serve(param.f0_length, param.fft_size / 2 + 1);
  for (int i = 0; i < param.f0_length; ++i) {
    for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
      one_filled_aperiodicity[i][j] = 1;
    }
  }

  int y_len = 1 + static_cast<int>((param.f0_length - 1) * param.frame_period /
                                   1000.0 * param.fs);
  std::unique_ptr<double[]> y = std::make_unique<double[]>(y_len);

  Synthesis(param.f0.get(), param.f0_length, spectrogram.get(),
            one_filled_aperiodicity.get(), param.fft_size, param.frame_period,
            param.fs, y_len, y.get());

  wavwrite(y.get(), y_len, param.fs, 16, filename);
}

void out_spectro(WorldParams &param, CStyle2DArrayCompat<double> &arr) {
  std::ofstream f("out.csv");

  for (int j = 0; j < param.fft_size / 2 + 1; ++j) {
    double bin_freq = static_cast<double>(j) * param.fs / param.fft_size;
    f << bin_freq << "," << arr[param.f0_length / 2][j] << "\n";
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::println(stderr, "usage: {} [filename] [output filename]", argv[0]);
    exit(1);
  }
  const char *input_file = argv[1];
  const char *output_file = argv[2];

  std::ifstream data(input_file);

  bool read_result;
  WorldParams param = WorldParams::read(data, read_result);

  std::print(stderr,
             "File: {}\n"
             "info: {} Hz / f0_len: {} / "
             "fft_size: {} \n",
             argv[1], param.fs, param.f0_length, param.fft_size);
  std::string csv_filename = std::string(output_file) + ".f0.csv";
  csv_out(csv_filename.c_str(), param.f0, param.f0_length);

  CStyle2DArrayCompat<double> phantom_spectral = PhantomShilhouette(param);
  output_with_whitenoize(output_file, param, phantom_spectral);
}
