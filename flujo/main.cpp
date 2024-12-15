#include <iostream>

#include "server/server.hpp"

int main()
{
    flujo::server::Server server{};
    if (!server.setup())
    {
        std::cout << "Failed to load configuration file" << std::endl;
        return -1;
    }

    server.run();

    return 0;
}
