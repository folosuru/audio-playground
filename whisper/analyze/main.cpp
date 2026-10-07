#include "./world_wrapper.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <print>
#include <string_view>

struct flags {
  bool dump_spectrogram = false;
  bool dump_f0 = false;
  int skip_rate = 1;
};

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::println(stderr, "usage: {} [filename] [output]", argv[0]);
    exit(1);
  }
  std::span<char *> args(argv, argc);
  const char *input_file = args[1];
  const char *output_file = args[2];

  flags flag;
  for (int i = 3; i < argc; ++i) {
    std::string_view arg = {args[i]};
    if (arg == "-s") {
      flag.dump_spectrogram = true;
    } else if (arg == "-f0") {
      flag.dump_f0 = true;
    }
  }

  std::string output_file_path =
      std::filesystem::path(output_file).replace_extension("");

  WorldParams param = world_alanyze(input_file);

  if (flag.dump_spectrogram) {
    std::string csv_path = output_file_path + "_spectrogram.csv";
    auto index_to_freq = [&](size_t i) {
      return static_cast<double>(i) * param.fs / param.fft_size;
    };
    auto index_to_time = [&](size_t i) { return param.time_axis[i]; };
    csv::file_out(csv_path.c_str(), param.spectrogram, "time", index_to_time,
                  index_to_freq,
                  /* sample step = */ 4, // 秒間50サンプルもあれば十分でしょ
                  /* frequency step =*/1);
  }
  if (flag.dump_f0) {
    std::string csv_path = output_file_path + "_f0.csv";
    csv::file_out(csv_path.c_str(),
                  csv::data_row{param.f0, static_cast<size_t>(param.f0_length)},
                  csv::axis_row{[&](size_t i) { return param.time_axis[i]; }});
  }

  std::println(stderr, "done");

  std::ofstream output(output_file);
  param.write(output);
}
