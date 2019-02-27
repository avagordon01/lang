#include <cstdint>
#include <cstdio>

extern "C" {
    uint32_t fibonacci(uint32_t);
}

int main() {
    printf("fibonacci(%u) = %u\n", 10, fibonacci(10));
}
