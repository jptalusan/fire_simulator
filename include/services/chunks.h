#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "data/location.h"

using json = nlohmann::json;

// Coordinate pair: lat, lon
std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> generate_osrm_table_chunks(
    const std::vector<Location>& sources,
    const std::vector<Location>& destinations,
    size_t chunk_size=100
);

void print_matrix(const std::vector<std::vector<double>>& matrix,
                  size_t max_rows = 10,
                  size_t max_cols = 10,
                  int precision = 2);

void write_matrix_to_csv(const std::vector<std::vector<double>>& matrix,
                         const std::string& filename,
                         int precision = 2,
                         bool add_headers = true);

void save_matrix_binary(const std::vector<std::vector<double>>& matrix,
                        const std::string& filename);

double* load_matrix_binary_flat(const std::string& filename, int& height, int& width);