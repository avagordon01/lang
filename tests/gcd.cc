#include <cstdint>
#include <cstdio>

extern "C" {
    uint32_t gcd(uint32_t, uint32_t);
}

int main() {
    printf("gcd(%u, %u) = %u\n", 100, 10, gcd(100, 10));
}
