#include <iostream>

extern "C" {
    float average(float, float);
}

int main() {
    std::cout << "average of 3.0 and 4.0: " << average(3.0, 4.0) << std::endl;
}
