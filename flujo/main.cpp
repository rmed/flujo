#include <iostream>

#include "config/loader.hpp"

int main()
{
    flujo::config::Loader lod{};
    std::cout << lod.load() << std::endl;
    return 0;
}
