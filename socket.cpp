#include <cstring>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "socket.h"

using namespace std;

Socket::Socket(int sfd, int stype, int port, bool dbg): sockfd {-1}, socktype {stype}, saved_addr {}, saved_port {port}, debug {dbg} {
    if(sfd >= 0) {
        // use the provided socket descriptor
        sockfd = sfd;
        saved_port = -1;
    } else {
        // create a new socket
        try {
            // if a valid port is provided, the address can be utilized for later operations like binding and connecting
            if(port >= 0) saved_addr.populate(socktype, port);
            else {
                saved_port = -1;
                saved_addr.populate(socktype);
            }
        } catch (address_list_err& err) {
            throw socket_exception {string {"socket exception: Socket: "} + err.what()};
        }

        // Operations borrowed from Beej's Guide
        addrinfo* itr = saved_addr.info;
        for(; itr != nullptr; itr = itr->ai_next) {
            // create socket
            sockfd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);

            if(sockfd == -1) {
                if(debug) cout<<"invalid socket address: "<<strerror(errno)<<endl;
                continue;
            } else break;
        }

        if(itr == nullptr) throw socket_exception{"socket exception: Socket: no valid address to create socket"};
    }

    if(debug) cout<<"Constructed socket with file descriptor: "<<sockfd<<endl;
}

Socket::Socket(Socket&& sock): sockfd {sock.sockfd}, socktype {sock.socktype}, saved_addr {}, saved_port {-1}, debug {sock.debug} {
    // manage ownership
    sock.sockfd = -1;
    if(debug) cout<<"Moving socket: "<<sockfd<<endl;
}

Socket& Socket::operator=(Socket&& sock) {
    close_socket();

    sockfd = sock.sockfd;
    socktype = sock.socktype;
    debug = sock.debug;

    // manage ownership
    sock.sockfd = -1;
    if(debug) cout<<"Moving socket: "<<sockfd<<endl;

    return *this;
}

void Socket::bind_socket(int port) {
    // Operations borrowed from Beej's Guide
    // prevent "Address already in use" error
    int yes = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        throw socket_exception {string {"socket exception: setsockopt: "} + strerror(errno)};
    }

    address_list bind_addr {};
    addrinfo* itr = nullptr;
    if(port == saved_port) {
        // use saved address if the port matches
        itr = saved_addr.info;
    } else {
        try {
            bind_addr.populate(socktype, port);
        } catch (address_list_err& err) {
            throw socket_exception {string {"socket exception: bind_socket: "} + err.what()};
        }

        itr = bind_addr.info;
    }

    // Operations borrowed from Beej's Guide
    for(; itr != nullptr; itr = itr->ai_next) {
        // bind the socket
        int status = bind(sockfd, itr->ai_addr, itr->ai_addrlen);

        if(status == -1) {
            if(debug) cout<<"invalid socket bind address: "<<strerror(errno)<<endl;
            continue;
        } else break;
    }
    if(itr == nullptr) throw socket_exception {"socket exception: bind_socket: no valid address to bind socket"};

    if(debug) cout<<"Bound socket "<<sockfd<<" to port: "<<port<<endl;
}

void Socket::listen_socket() {
    // Operations borrowed from Beej's Guide
    // listen on the socket
    int status = listen(sockfd, BACKLOG);

    if(status == -1) {
        throw socket_exception {string {"socket exception: listen_socket: "} + strerror(errno)};
    }

    if(debug) cout<<"Listening on socket "<<sockfd<<endl;
}

void Socket::connect_socket(int port) {
    address_list connect_addr {};
    addrinfo* itr = nullptr;
    if(port == saved_port) {
        // use saved address if the port matches
        itr = saved_addr.info;
    } else {
        try {
            connect_addr.populate(socktype, port);
        } catch (address_list_err& err) {
            throw socket_exception {string {"socket exception: connect_socket: "} + err.what()};
        }

        itr = connect_addr.info;
    }

    // Operations borrowed from Beej's Guide
    for(; itr != nullptr; itr = itr->ai_next) {
        // connect to the provided port
        int status = connect(sockfd, itr->ai_addr, itr->ai_addrlen);

        if(status == -1) {
            if(debug) cout<<"invalid socket connect address: "<<strerror(errno)<<endl;
        } else break;
    }
    if(itr == nullptr) throw socket_exception {"socket exception: connect_socket: no valid address to connect socket"};

    // save connected port
    connected_port = port;
    if(debug) cout<<"Connected socket "<<sockfd<<" to port: "<<port<<endl;
}

Socket Socket::accept_socket() {
    // Operations borrowed from Beej's Guide
    sockaddr_storage connected_to;
    socklen_t sin_size = sizeof(connected_to);

    // accept connection and create a child socket
    int childfd = accept(sockfd, (sockaddr*) &connected_to, &sin_size);

    if(childfd == -1) {
        throw socket_exception {string {"socket exception: accept_socket: "} + strerror(errno)};
    }

    Socket child {childfd, socktype, -1, debug};

    // save connected port
    child.connected_port = ntohs(((sockaddr_in*) &connected_to)->sin_port);
    if(debug) cout<<"Socket "<<sockfd<<" established connection with port: "<<child.connected_port<<endl;

    return child;
};

void Socket::send_info(const string& s) {
    // Operations borrowed from Beej's Guide
    // send information to the connected socket
    int sent = send(sockfd, s.c_str(), s.size(), 0);

    if(sent == -1) {
        throw socket_exception {string {"socket exception: send_info: "} + strerror(errno)};
    }

    if(debug) cout<<"Sent "<<sent<<" bytes of message: "<<s<<endl;
}

string Socket::recv_info() {
    // Operations borrowed from Beej's Guide
    char data[MAXDATASIZE];    

    // receive information from the connected socket
    int received = recv(sockfd, data, MAXDATASIZE - 1, 0);

    if(received == -1) {
        throw socket_exception {string {"socket exception: recv_info: "} + strerror(errno)};
    }

    data[received] = 0;
    string rec {data};

    if(debug && received > 0) cout<<"Received "<<received<<" bytes of message: "<<rec<<endl;
    return rec;
}

void Socket::send_info_to(int port, const string& s) {
    // Operations borrowed from Beej's Guide
    // save address for future send operations
    if(port != saved_port) {
        try {
            saved_addr.free();
            saved_addr.populate(socktype, port);
        } catch (address_list_err& err) {
            throw socket_exception {string {"socket exception: send_info_to: "} + err.what()};
        }

        saved_port = port;
    }

    int sent = -1;
    addrinfo* itr = saved_addr.info;
    for(; itr != nullptr; itr = itr->ai_next) {
        // send information to the provided address
        sent = sendto(sockfd, s.c_str(), s.size(), 0, itr->ai_addr, itr->ai_addrlen);

        if(sent == -1) {
            if(debug) cout<<"invalid send to address: "<<strerror(errno)<<endl;
            continue;
        } else break;
    }
    if(itr == nullptr) throw socket_exception {"socket exception: send_info_to: no valid address to send to"};

    if(debug) cout<<"Sent "<<sent<<" bytes of message: "<<s<<endl;
}

msg_port Socket::recv_info_from() {
    // Operations borrowed from Beej's Guide
    // struct to save sender identity
    sockaddr_storage connected_to;
    socklen_t sin_size = sizeof(connected_to);

    char data[MAXDATASIZE];    

    // receive information as well as sender identity
    int received = recvfrom(sockfd, data, MAXDATASIZE - 1, 0, (sockaddr*) &connected_to, &sin_size);

    if(received == -1) {
        throw socket_exception {string {"socket exception: recv_info_from: "} + strerror(errno)};
    }

    data[received] = 0;

    // return both the data and the sender port number
    msg_port rec {data, ntohs(((sockaddr_in*) &connected_to)->sin_port)};

    if(debug) cout<<"Socket "<<sockfd<<" received "<<received
                  <<" bytes from port "<<rec.port
                  <<" of message: "<<rec.msg<<endl;

    return rec;
}

int Socket::bound_port() {
    sockaddr_storage self_addr;
    socklen_t self_size = sizeof(self_addr);

    // retrieve connected socket information
    int status = getsockname(sockfd, (sockaddr*) &self_addr, &self_size);

    if(status == -1) {
        throw socket_exception {string {"socket exception: bound_port: "} + strerror(errno)};
    }

    // return the connected port number
    return ntohs(((sockaddr_in*) &self_addr)->sin_port);
}

void Socket::close_socket() {
    // close if valid
    if(sockfd >= 0) {
        if(debug) cout<<"Closing socket: "<<sockfd<<endl;
        
        close(sockfd);
        sockfd = -1;
    }
}

void Socket::reap_dead_processes() {
    // Operations borrowed from Beej's Guide
    struct sigaction sa {};

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if(sigaction(SIGCHLD, &sa, nullptr) == -1) {
        throw socket_exception {string {"socket exception: reap_dead_processes: "} + strerror(errno)};
    }

    if(debug) cout<<"Reaped dead processes\n";
}

Socket::~Socket() {
    close_socket();
}

socket_exception::socket_exception(const string& err) : std::runtime_error{err} {}

void sigchld_handler(int s) {
    // Operations borrowed from Beej's Guide
    int saved_errno = errno;
    while(waitpid(-1, nullptr, WNOHANG) > 0);
    errno = saved_errno;
}

