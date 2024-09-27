#include <stdexcept>
#include <netdb.h>

/*
 * Class address_list stores and manipulates internet addresses
 * in the form of a pointer to struct addrinfo
 */
class address_list {
public:
    addrinfo* info;

    address_list();

    // populate address with loopback address, the provided port and the provided socket type
    void populate(int socktype, int port = 0);

    // free occupied memory
    void free();

    ~address_list();
};

class address_list_err : public std::runtime_error {
public:
    address_list_err(const std::string& err); 
};
