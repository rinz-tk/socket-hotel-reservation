#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "socket.h"
#include "backend.h"
#include "constants.h"

using namespace std;
using namespace socket_constants;


class backend_exception : public runtime_error {
public:
    backend_exception(const string& err) : runtime_error{err} {}; 
};


// read the provided file and save the room counts
unordered_map<string, int> read_status(const string& filename) {
    unordered_map<string, int> room_status {};
    ifstream f {filename};

    if(!f.good()) throw backend_exception {"backend exception: read_status: file " + filename + " does not exist"};

    string room;
    int number;
    string next_line;
    while(f.good() && getline(f, room, ',') && f >> number) {
        room_status[room] = number;
        getline(f, next_line);
    }

    return room_status;
}


// send the room statuses to the main server
void send_list(Socket& sock, const unordered_map<string, int>& room_status) {
    string msg {};

    // send the information in pieces so as to not exceed the receive buffer size
    for(const pair<string, int>& r : room_status) {
        string m = r.first + ',' + to_string(r.second) + '\n';

        if(msg.size() + m.size() > Socket::MAXDATASIZE - 3) {
            sock.send_info_to(serverM_backend, msg);
            msg = m;
        } else msg += m;
    }

    // send the code to signify that all the status information has been sent
    sock.send_info_to(serverM_backend, msg + ',' + FINISH_STATUS);
}


// search for the provided room and relay the information to the main server
void availability_request(Socket& sock, const char server_name, const unordered_map<string, int>& room_status, const string& room) {
    // lookup the room status for the room
    unordered_map<string, int>::const_iterator available = room_status.find(room);

    if(available == room_status.end()) {
        cout<<"Not able to find the room layout.\n";
        sock.send_info_to(serverM_backend, ROOM_NOT_FOUND);
    } else if(available->second > 0) {
        cout<<"Room "<<room<<" is available.\n";
        sock.send_info_to(serverM_backend, ROOM_AVAILABLE);
    } else {
        cout<<"Room "<<room<<" is not available.\n";
        sock.send_info_to(serverM_backend, ROOM_NOT_AVAILABLE);
    }

    cout<<"The Server "<<server_name<<" finished sending the response to the main server.\n";
}


// search for the provided room, decrement the count if available, and relay the information to the main server
void reservation_request(Socket& sock, const char server_name, unordered_map<string, int>& room_status, const string& room) {
    // lookup the room status for the room
    unordered_map<string, int>::iterator available = room_status.find(room);

    if(available == room_status.end()) {
        cout<<"Cannot make a reservation. Not able to find the room layout.\n";
        sock.send_info_to(serverM_backend, ROOM_NOT_FOUND);
    } else if(available->second > 0) {
        // decrement the room count
        available->second = available->second - 1;
        cout<<"Successful reservation. The count of Room "<<room<<" is now "<<available->second<<".\n";

        // send the new room count to the main server
        sock.send_info_to(serverM_backend, string {ROOM_AVAILABLE} + '\n' + to_string(available->second));

        cout<<"The Server "<<server_name<<" finished sending the response and the updated room status to the main server.\n";
        return;
    } else {
        cout<<"Cannot make a reservation. Room "<<room<<" is not available.\n";
        sock.send_info_to(serverM_backend, ROOM_NOT_AVAILABLE);
    }

    cout<<"The Server "<<server_name<<" finished sending the response to the main server.\n";
}


// a backend server is responsible for reading and storing room status information from a file,
// and communicating with the main server to satisfy user requests
int run_backend(const char server_name, const int sock_port, const string& filename) {
    constexpr bool debug = false;

    try {
        // create UDP socket and bind it
        Socket sock {-1, SOCK_DGRAM, sock_port, debug};
        sock.bind_socket(sock_port);

        cout<<"The Server "<<server_name<<" is up and running using UDP on port "<<sock_port<<".\n";

        // the room status information is stored as a hash table, mapping the rooms to their counts
        unordered_map<string, int> room_status = read_status(filename);
        send_list(sock, room_status);

        cout<<"The Server "<<server_name<<" has sent the room status to the main server.\n";

        while(true) {
            msg_port request = sock.recv_info_from();

            // invalidate information not received from the main server
            if(request.port != serverM_backend) {
                cout<<"The Server "<<server_name<<" has received a request from an unknown server on UDP with port "<<request.port<<".\n";
                continue;
            }

            string request_type, room;
            istringstream sstream {request.msg};

            if(!getline(sstream, request_type)) {
                cout<<"The Server "<<server_name<<" has received a request with a missing request type using UDP over port "<<sock_port<<".\n";
                sock.send_info_to(serverM_backend, REQUEST_EMPTY);
                continue;
            }

            if(!getline(sstream, room)) {
                cout<<"The Server "<<server_name<<" has received a request with a missing room using UDP over port "<<sock_port<<".\n";
                sock.send_info_to(serverM_backend, ROOM_EMPTY);
                continue;
            }

            if(request_type == AVAILABILITY_REQUEST) {
                cout<<"The Server "<<server_name<<" received an availability request from the main server.\n";
                availability_request(sock, server_name, room_status, room);
            } else if(request_type == RESERVATION_REQUEST) {
                cout<<"The Server "<<server_name<<" received a reservation request from the main server.\n";
                reservation_request(sock, server_name, room_status, room);
            } else {
                cout<<"The Server "<<server_name<<" has received an invalid request type using UDP over port "<<sock_port<<".\n";
                sock.send_info_to(serverM_backend, INVALID_REQUEST);
            }
        }

        return 0;

    } catch(socket_exception& se) {
        cout<<se.what()<<endl;
        return 1;
    } catch(backend_exception& be) {
        cout<<be.what()<<endl;
        return 1;
    }
}
