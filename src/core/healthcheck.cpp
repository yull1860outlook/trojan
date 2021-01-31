#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#include <future>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

std::string isPortOpen(const std::string &domain, const std::string &port)
{
    addrinfo *result;
    addrinfo hints{}; 
    hints.ai_family = AF_UNSPEC;   // either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; 
    char addressString[INET6_ADDRSTRLEN];
    const char *retval = nullptr;
    if (0 != getaddrinfo(domain.c_str(), port.c_str(), &hints, &result)) {
        return "";
    }
    for (addrinfo *addr = result; addr != nullptr; addr = addr->ai_next) {
        int handle = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (handle == -1) {
            continue;
        }
        if (connect(handle, addr->ai_addr, addr->ai_addrlen) != -1) {
            switch(addr->ai_family) {
                case AF_INET:
                    retval = inet_ntop(addr->ai_family, &(reinterpret_cast<sockaddr_in *>(addr->ai_addr)->sin_addr), addressString, INET6_ADDRSTRLEN);
                    break;
                case AF_INET6:
                    retval = inet_ntop(addr->ai_family, &(reinterpret_cast<sockaddr_in6 *>(addr->ai_addr)->sin6_addr), addressString, INET6_ADDRSTRLEN);
                    break;
                default:
                    // unknown family
                    retval = nullptr;
            }
            close(handle);
            break;
        }
    }
    freeaddrinfo(result);
    return retval==nullptr ? "" : domain + ":" + retval + "\n";
}

int testmain(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " domains.txt port threads timeout output.txt\n";
        std::exit(-1);
    }

    std::string domains_file = argv[1];
    std::string output_file  = argv[5];
    std::string port         = argv[2];
    int threads              = atoi(argv[3]);
    int timeout              = atoi(argv[4]);

    std::ifstream in{argv[1]};
    std::vector<std::string> domains;
    std::copy(std::istream_iterator<std::string>(in),
        std::istream_iterator<std::string>(), 
        std::back_inserter(domains));

    std::ofstream myfile{output_file};

    std::vector<std::future<std::string>> results;
    for (const auto &domain: domains) {
        results.push_back(std::async(isPortOpen, domain, port));
    }
    std::for_each(results.begin(), results.end(), 
        [&myfile](std::future<std::string>& f){myfile << f.get();});

    myfile.close();
}