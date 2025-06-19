
#include <exception>
#include <stdexcept>
#include <string>

class OSRMError : public std::runtime_error {
public:
    explicit OSRMError(const std::string& msg="OSRM Error")
        : std::runtime_error(msg) {}
};

class MismatchError : public std::runtime_error {
public:
    explicit MismatchError(const std::string& msg="Mismatch error")
        : std::runtime_error(msg) {}
};

class UnknownValueError : public std::runtime_error {
public:
    explicit UnknownValueError(const std::string& msg="Unknown value error")
        : std::runtime_error(msg) {}
};

class InvalidIncidentError : public std::runtime_error {
public:
    explicit InvalidIncidentError(const std::string& msg="Invalid incident error")
        : std::runtime_error(msg) {}
};

class InvalidStationError : public std::runtime_error {
public:
    explicit InvalidStationError(const std::string& msg="Invalid station error")
        : std::runtime_error(msg) {}
};