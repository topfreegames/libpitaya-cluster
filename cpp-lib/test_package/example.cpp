#include <pitaya.h>
#include <iostream>

int main()
{
    pitaya::Server server(pitaya::Server::Kind::Frontend, "my-id", "my-type");
    std::cout << "Server: " << std::endl;
    std::cout << "  id: " << server.Id() << std::endl;
    std::cout << "  type: " << server.Type() << std::endl;
}
