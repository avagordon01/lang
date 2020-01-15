#pragma once

#include <string>
#include <iostream>

template<typename ... Ts>
[[noreturn]] static void error(Ts ... args) {
    ((std::cerr << args << " "), ...) << std::endl;
    exit(EXIT_FAILURE);
}

template<typename ... Ts>
static void info(Ts ... args) {
    ((std::cerr << args << " "), ...) << std::endl;
}
