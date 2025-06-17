#include <services/chunks.h>
#include "data/location.h"

// libcurl write callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string fetch_osrm_response(const std::string& full_url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // Optional: curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL failed: " << curl_easy_strerror(res) << "\n";
        }
        curl_easy_cleanup(curl);
    }
    return response;
}

// Generate OSRM table queries: all sources with destination chunks
std::vector<std::vector<double>> generate_osrm_table_chunks(
    const std::vector<Location>& sources,
    const std::vector<Location>& destinations,
    const std::string& feature,
    size_t chunk_size
) {
    const std::string base_url = "http://localhost:8080";
    size_t num_sources = sources.size();
    size_t num_destinations = destinations.size();

    // Initialize final durations matrix [sources][destinations]
    std::vector<std::vector<double>> full_matrix(num_sources, std::vector<double>(num_destinations, -1.0));

    // Precompute source strings
    std::vector<std::string> source_strs;
    for (const auto& s : sources) source_strs.push_back(locationToString(s));

    for (size_t i = 0; i < destinations.size(); i += chunk_size) {
        std::vector<std::string> all_coords = source_strs;
        std::vector<std::string> chunk_strs;
        std::vector<size_t> dest_indices;

        // Build destination chunk
        for (size_t j = i; j < std::min(i + chunk_size, destinations.size()); ++j) {
            chunk_strs.push_back(locationToString(destinations[j]));
            all_coords.push_back(chunk_strs.back());
            dest_indices.push_back(sources.size() + (j - i));
        }

        // Build coordinates string
        std::ostringstream coords_param;
        for (size_t k = 0; k < all_coords.size(); ++k) {
            coords_param << all_coords[k];
            if (k < all_coords.size() - 1)
                coords_param << ";";
        }

        // Sources: 0..num_sources-1
        std::ostringstream sources_param;
        for (size_t s = 0; s < num_sources; ++s)
            sources_param << s << (s < num_sources - 1 ? ";" : "");

        // Destinations: indices after sources
        std::ostringstream destinations_param;
        for (size_t d = 0; d < dest_indices.size(); ++d)
            destinations_param << dest_indices[d] << (d < dest_indices.size() - 1 ? ";" : "");

        // Build URL
        std::string full_url = base_url + "/table/v1/driving/" + coords_param.str() +
            "?sources=" + sources_param.str() +
            "&destinations=" + destinations_param.str() +
            "&annotations=duration,distance";

        // Fetch and parse JSON
        std::string response = fetch_osrm_response(full_url);
        auto json_resp = json::parse(response);

        if (json_resp["code"] != "Ok") {
            std::cerr << "OSRM error: " << json_resp["code"] << "\n";
            continue;
        }
        
        auto durations = json_resp[feature];

        // Fill values into full_matrix
        for (size_t row = 0; row < durations.size(); ++row) {
            for (size_t col = 0; col < durations[row].size(); ++col) {
                size_t dst_index = i + col;
                full_matrix[row][dst_index] = durations[row][col].get<double>();
            }
        }

        std::cout << "Processed destinations " << i << " to " << std::min(i + chunk_size, destinations.size()) - 1 << "\n";
    }
    return full_matrix;
}

void print_matrix(const std::vector<std::vector<double>>& matrix,
                  size_t max_rows,
                  size_t max_cols,
                  int precision) {
    size_t rows = matrix.size();
    size_t cols = matrix.empty() ? 0 : matrix[0].size();

    size_t display_rows = std::min(rows, max_rows);
    size_t display_cols = std::min(cols, max_cols);

    std::cout << std::fixed << std::setprecision(precision);

    // Header
    std::cout << "      ";
    for (size_t j = 0; j < display_cols; ++j)
        std::cout << "Dst" << std::setw(3) << j << " ";
    if (display_cols < cols)
        std::cout << "...";
    std::cout << "\n";

    // Rows
    for (size_t i = 0; i < display_rows; ++i) {
        std::cout << "Src" << std::setw(3) << i << " ";
        for (size_t j = 0; j < display_cols; ++j) {
            std::cout << std::setw(6) << matrix[i][j] << " ";
        }
        if (display_cols < cols)
            std::cout << "...";
        std::cout << "\n";
    }

    if (display_rows < rows)
        std::cout << "...\n";
}


#include <fstream>
#include <iomanip>

void write_matrix_to_csv(const std::vector<std::vector<double>>& matrix,
                         const std::string& filename,
                         int precision,
                         bool add_headers) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    file << std::fixed << std::setprecision(precision);

    size_t rows = matrix.size();
    size_t cols = rows > 0 ? matrix[0].size() : 0;

    // Column headers
    if (add_headers) {
        file << " ";
        for (size_t j = 0; j < cols; ++j) {
            file << ",Dst" << j;
        }
        file << "\n";
    }

    for (size_t i = 0; i < rows; ++i) {
        if (add_headers)
            file << "Src" << i;
        else
            file << i;

        for (size_t j = 0; j < cols; ++j) {
            file << "," << matrix[i][j];
        }
        file << "\n";
    }

    file.close();
    std::cout << "Matrix written to: " << filename << "\n";
}
