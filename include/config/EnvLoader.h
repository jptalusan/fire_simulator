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
    // Singleton pattern: get the shared instance
    static std::shared_ptr<EnvLoader> getInstance() {
        return instance;
    }
    
    // Initialize the singleton instance once (call this once at startup)
    static void init(const std::string& filename = ".env") {
        instance = std::shared_ptr<EnvLoader>(new EnvLoader(filename));
    }

    std::string get(const std::string& key, const std::string& default_val = "") const {
        auto it = env_map.find(key);
        if (it != env_map.end()) return it->second;
        return default_val;
    }

    // Optional: explicit cleanup method (for tests)
    static void cleanup() {
        instance.reset();  // This will delete the object if it's the last reference
    }

private:
    // Private constructor for singleton pattern
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

    std::unordered_map<std::string, std::string> env_map;
    inline static std::shared_ptr<EnvLoader> instance = nullptr;
};

#endif // ENVLOADER_H
