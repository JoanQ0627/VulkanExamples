#include "Triangle.h"
#include <iostream>
#include <stdexcept>

int main() 
{
    Triangle app;

    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}