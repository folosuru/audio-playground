#include "../Utils.hpp"
#include <fstream>

using namespace csv;

void csv::file_out(const char *filename, data_row data, axis_row axis,
                   int step) {
  std::ofstream file(filename);

  if (axis.name) {
    file << axis.name.value() << ",";
  } else if (data.name) {
    file << ",";
  }

  for (size_t i = 0; i < data.len; i += step) {
    file << axis.axis_gen(i) << ",";
  }
  file << "\n";

  if (data.name) {
    file << data.name.value() << ",";
  } else if (axis.name) {
    file << ",";
  }

  for (size_t i = 0; i < data.len; i += step) {
    file << data.array[i] << ", ";
  }
}

void csv::file_out(const char *filename, CStyle2DArrayCompat<double> &array,
                   const char *first_cell_data,
                   std::function<double(size_t)> d1_axis_gen,
                   std::function<double(size_t)> d2_axis_gen, int d1_step,
                   int d2_step) {
  std::ofstream file(filename);

  file << first_cell_data;
  for (size_t i = 0; i < array.array_size(); i += d2_step) {
    file << "," << d2_axis_gen(i);
  }
  file << "\n";

  for (size_t i = 0; i < array.index_size(); i += d1_step) {
    file << d1_axis_gen(i);

    auto current_array = array.get_array(i);
    auto end = current_array.end();
    for (auto iter = current_array.begin(); iter != end; iter += d2_step) {
      file << "," << (*iter);
    }
    file << "\n";
  }
}
