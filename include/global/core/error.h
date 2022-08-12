#pragma once
#include <stdexcept>
#include <string>

class error: std::runtime_error {

public:
    error(std::string const& s) : std::runtime_error(s) {}
    error(char const* s) : std::runtime_error(s) {}

    inline std::string message() const {
        return std::string("[!] ") + this->what();
    }
};
