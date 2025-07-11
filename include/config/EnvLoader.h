#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

class EnvLoader {
public:
    explicit EnvLoader(const std::string& filename = ".env") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            // print error
            std::cerr << "Error: Could not open .env file: " << filename << std::endl;
            return;  // silently ignore if no .env file found
        }
        std::string line;
        while (std::getline(file, line)) {
            // Remove whitespace
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            auto delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) continue;

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            env_map[key] = value;
        }
    }

    std::string get(const std::string& key, const std::string& default_val = "") const {
        auto it = env_map.find(key);
        if (it != env_map.end()) return it->second;
        return default_val;
    }
    void set(const std::string& key, const std::string& value) {
        env_map[key] = value;
    }

private:
    std::unordered_map<std::string, std::string> env_map;
};

#endif // ENVLOADER_H
