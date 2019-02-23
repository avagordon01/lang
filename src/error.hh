#pragma once

#include <string>
#include <iostream>

static void error(std::string s) {
    std::cerr << s << std::endl;
    exit(EXIT_FAILURE);
}
