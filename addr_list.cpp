#include <stdexcept>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "addr_list.h"

using namespace std;

address_list::address_list(): info {nullptr} {}

// Operations borrowed from Beej's Guide
void address_list::populate(int socktype, int port) {
    int status;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;

    string ps = to_string(port);

    status = getaddrinfo("localhost", ps.c_str(), &hints, &info);

    if(status != 0) {
        throw address_list_err { string{"address_list error: "} + string{gai_strerror(status)} };
    }
}

void address_list::free() {
    // free if valid
    if(info != nullptr) {
        freeaddrinfo(info);
        info = nullptr;
    }
}

address_list::~address_list() {
    free();
}

address_list_err::address_list_err(const string& err) : std::runtime_error{err} {}
