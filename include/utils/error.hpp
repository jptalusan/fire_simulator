
#include <exception>

class OSMError : public std::exception {
public:
    const char* what() const noexcept override {
        return "Something went wrong in OSRM";
    }
};