#pragma once

#include <string>
#include <iostream>
#include <sstream>

template<typename ... Ts>
[[noreturn]] static void error(Ts ... args) {
    ((std::cerr << args << " "), ...) << std::endl;
    exit(EXIT_FAILURE);
}

template<typename ... Ts>
static void info(Ts ... args) {
    ((std::cerr << args << " "), ...) << std::endl;
}

template<typename... Ts>
std::string string_error(Ts... args) {
    std::stringstream ss{};
    ((ss << args << " "), ...);
    return ss.str();
}

/*
[[noreturn]] void fatal_error(std::string e) {
    std::cerr << e << std::endl;
    exit(1);
}
*/
