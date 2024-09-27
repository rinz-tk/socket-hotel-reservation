#include <netdb.h>

#include "addr_list.h"

class Socket;

// struct to return poth a message and a port from a method
struct msg_port {
    std::string msg;
    int port;
};

/*
 * class Socket represents a unique socket
 */
class Socket {
private:
    // socket file descriptor
    int sockfd;
    // socket type, TCP or UDP
    int socktype;

    // save address information for use in operations such as binding and connecting
    address_list saved_addr;
    int saved_port;

    // determines whether to display socket debug information
    bool debug;

    // number of entries allowed in a listen queue
    constexpr static int BACKLOG = 10;

public:
    // buffer size while receiving information through a socket
    constexpr static int MAXDATASIZE = 1024;

    // constructor
    Socket(int sfd, int stype, int port = -1, bool debug = false);

    // disallow copy operations to maintain unique ownership of sockets
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // allow move operations to maintain unique ownership of sockets
    Socket(Socket&&);
    Socket& operator=(Socket&&);

    // bind socket to provided port
    void bind_socket(int port);

    // begin listening on socket
    void listen_socket();

    // attempt connection to a server socket
    void connect_socket(int port);

    // accept connection from a client socket
    Socket accept_socket();

    // send string information to a connected TCP socket
    void send_info(const std::string& s);

    // receive string information from a connected TCP socket
    std::string recv_info();

    // send information to the provided port through a UDP socket
    void send_info_to(int port, const std::string& s);

    // receive information through a UDP socket
    // returns both the received information and the port of the sender
    msg_port recv_info_from();

    // returns the port number to which the socket is bound
    int bound_port();

    // close the socket and invalidate the socket file descriptor
    void close_socket();

    // reap zombie processes
    void reap_dead_processes();

    // saves the connected port for a TCP connection
    int connected_port {-1};

    // destroy the socket and release the memory
    ~Socket();
};

class socket_exception : public std::runtime_error {
public:
    socket_exception(const std::string& err); 
};

void sigchld_handler(int s);
