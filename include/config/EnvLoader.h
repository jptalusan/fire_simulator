#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class EnvLoader {
public:
    // Singleton pattern: get the shared instance
    static std::shared_ptr<EnvLoader> getInstance() {
        return instance;
    }
    
    // Initialize the singleton instance once with JSON string
    static void init(const std::string& json_config, const std::string& type = "json") {
        if (type == "json") {
            instance = std::shared_ptr<EnvLoader>(new EnvLoader(json_config, true));
        } else if (type == "file") {
            instance = std::shared_ptr<EnvLoader>(new EnvLoader(json_config, false));
        } else {
            std::cerr << "Error: Invalid type '" << type << "'. Use 'json' or 'file'." << std::endl;
        }
    }

    // Backward compatibility: initialize with .env file (old behavior)
    static void initFromFile(const std::string& filename = ".env") {
        instance = std::shared_ptr<EnvLoader>(new EnvLoader(filename, false));
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
    // Private constructor for singleton pattern - supports both JSON and file input
    explicit EnvLoader(const std::string& input, bool is_json = true) {
        if (is_json) {
            // Parse JSON configuration
            try {
                auto json_data = json::parse(input);
                
                // Iterate through JSON object and populate env_map
                for (auto& [key, value] : json_data.items()) {
                    if (value.is_string()) {
                        env_map[key] = value.get<std::string>();
                    } else if (value.is_number()) {
                        env_map[key] = std::to_string(value.get<double>());
                    } else if (value.is_boolean()) {
                        env_map[key] = value.get<bool>() ? "true" : "false";
                    } else {
                        // For other types, convert to string representation
                        env_map[key] = value.dump();
                    }
                }
            } catch (const json::exception& e) {
                std::cerr << "Error: Failed to parse JSON configuration: " << e.what() << std::endl;
                // Continue with empty env_map if JSON parsing fails
            }
        } else {
            // Parse .env file (original functionality)
            std::ifstream file(input);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open .env file: " << input << std::endl;
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
    }

    std::unordered_map<std::string, std::string> env_map;
    inline static std::shared_ptr<EnvLoader> instance = nullptr;
};

#endif // ENVLOADER_H
