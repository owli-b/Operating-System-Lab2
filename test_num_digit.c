#include "types.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char* argv[]) {
    int n = atoi(argv[1]), prev_ebx;
    // clang-format off
    asm volatile(
        "movl %%ebx, %0;"
        "movl %1, %%ebx;"
        : "=r"(prev_ebx)
        : "r"(n));
    int result = count_num_of_digits(n);
    asm volatile(
        "movl %0, %%ebx;"
        :: "r"(prev_ebx));
    // clang-format on
    printf(1, "The number %d has %d digits.\n", n, result);
    exit();
}
