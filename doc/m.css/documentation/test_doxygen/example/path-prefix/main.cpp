#include <iostream>

#include "configure.h"

int main() {
    #ifdef SAY_HELLO
    std::cout << "hello?" << std::endl;
    #endif
}
