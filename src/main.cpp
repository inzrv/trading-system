#include "common/runtime.h"

#include <iostream>

int main()
{
    try {
        Runtime runtime;
        runtime.run();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
