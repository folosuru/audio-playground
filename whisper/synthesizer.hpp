#ifndef INCLUDE_WHISPER_SYNTHESIZER_HPP_
#define INCLUDE_WHISPER_SYNTHESIZER_HPP_

#include <memory>
std::unique_ptr<double[]> freq_to_wave(double *freq, int len, int &out);

#endif // INCLUDE_WHISPER_SYNTHESIZER_HPP_
