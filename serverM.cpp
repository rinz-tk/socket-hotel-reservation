#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "socket.h"
#include "encrypt.h"
#include "constants.h"

using namespace std;
using namespace socket_constants;


class server_exception : public runtime_error {
public:
    server_exception(const string& err) : runtime_error{err} {}; 
};


// receive room status from all the backend servers
unordered_map<string, pair<int, int>> get_room_status(Socket& server_sock, const map<int, char>& backend) {
    unordered_map<string, pair<int, int>> room_status {};

    // map track keeps track of which backend servers have finished transmitting their status
    map<int, bool> track {};
    for(const pair<int, char>& i : backend) track.insert({i.first, false});

    // keep receiving status information until all the backend servers have finished transmitting
    while(any_of(track.begin(), track.end(), [](const pair<int, bool>& s) { return !s.second; })) {
        msg_port rec = server_sock.recv_info_from();

        map<int, bool>::iterator tf = track.find(rec.port);
        if(tf != track.end() && tf->second == false) {
            string room;
            int number;
            string next_line;
            istringstream sstream {rec.msg};
            while(sstream.good() && getline(sstream, room, ',') && sstream >> number) {
                if(room == "" && to_string(number) == FINISH_STATUS) {
                    // backend server has finished transmission
                    tf->second = true;
                    cout<<"The main server has received the room status from Server "<<backend.find(rec.port)->second<<" using UDP over port "<<serverM_backend<<".\n";
                    break;

                } else {
                    // save room status information, mapping a room to its corresponding server (port number) and the count of the room
                    room_status[room] = {rec.port, number};
                    getline(sstream, next_line);

                }
            }
        }
    }

    return room_status;
}


// read and store the encrypted usernames and passwords information from the given file
unordered_map<string, string> get_user_info(const string& user_filename) {
    unordered_map<string, string> user_info {};
    ifstream f {user_filename};

    if(!f.good()) throw server_exception {"server exception: get_user_info: file " + user_filename + " does not exist"};

    string username, password;
    while(getline(f, username, ',') && f.ignore() && getline(f, password)) {
        user_info[username] = password;
    }

    return user_info;
}


// authenticate the user credentials by comparing it to the stored user information
bool authenticate(Socket& child, const unordered_map<string, string>& user_info, bool& member, string& username, bool& open) {
    bool success = false;
    string auth = child.recv_info();

    string password;
    istringstream sstream {auth};

    // mark connection as closed if an empty string is received
    if(!getline(sstream, username)) {
        cout<<"The client with port "<<child.connected_port<<" has closed the connection.\n";
        open = false;
        return false;
    }

    if(getline(sstream, password)) {
        // a password implies a member request
        cout<<"The main server received the authentication for "<<username<<" using TCP over port "<<serverM_client<<".\n";
        
        // lookup the user info for the valid user credentials
        unordered_map<string, string>::const_iterator saved_info = user_info.find(username);
        if(saved_info != user_info.end()) {
            if(saved_info->second == password) {
                member = true;
                success = true;
                child.send_info(VALID_MEMBER);
            } else {
                child.send_info(INVALID_PASSWORD);
            }
        } else {
            child.send_info(INVALID_USER);
        }
        cout<<"The main server sent the authentication result to the client.\n";
    } else {
        // an empty password implies a guest request
        cout<<"The main server has received the guest request for "<<username<<" using TCP over port "<<serverM_client<<".\n";
        success = true;
        member = false;
        cout<<"The main server accepts "<<username<<" as a guest.\n";

        child.send_info(VALID_GUEST);

        cout<<"The main server sent the guest response to the client.\n";
    }

    return success;
}


// satisfy availability requests from a client by querying the appropriate backend server
void availability_request(Socket& child, Socket& server_sock, const map<char, int>& router, const string& request, const string& room) {
    // the first character of the room is the name of the related backend server
    const char server_name = room[0];
    map<char, int>::const_iterator route_server = router.find(server_name);

    if(route_server == router.end()) {
        cout<<"The main server found no corresponding Server for room "<<room<<".\n";
        child.send_info(ROOM_NOT_FOUND);
    } else {
        server_sock.send_info_to(route_server->second, request);
        cout<<"The main server sent a request to Server "<<server_name<<".\n";

        msg_port response = server_sock.recv_info_from();

        if(response.port != route_server->second) {
            cout<<"The main server has received a response from an unexpected Server with port "<<response.port<<".\n";
            child.send_info(ROOM_NOT_FOUND);
        } else {
            cout<<"The main server received the response from Server "<<server_name<<" using UDP over port "<<serverM_backend<<".\n";

            if(response.msg != "") {
                child.send_info(response.msg);
            } else {
                cout<<"The backend Server "<<server_name<<" has sent an empty response.\n";
                child.send_info(ROOM_NOT_FOUND);
            }
        }
    }

    cout<<"The main server sent the availability information to the client.\n";
}


// satisfy reservation requests from a client by querying the appropriate backend server
void create_reservation(Socket& child, Socket& server_sock, const map<char, int>& router, unordered_map<string, pair<int, int>>& room_status, const string& request, const string& room, const bool member, const string& username) {
    // a guest cannot make a reservation
    if(!member) {
        cout<<username<<" cannot make a reservation.\n";
        child.send_info(USER_NOT_MEMBER);

        cout<<"The main server sent the error message to the client.\n";
        return;
    }

    // the first character of the room is the name of the related backend server
    const char server_name = room[0];
    map<char, int>::const_iterator route_server = router.find(server_name);

    if(route_server == router.end()) {
        cout<<"The main server found no corresponding Server for room "<<room<<".\n";
        child.send_info(ROOM_NOT_FOUND);
    } else {
        server_sock.send_info_to(route_server->second, request);
        cout<<"The main server sent a request to Server "<<server_name<<".\n";

        msg_port response = server_sock.recv_info_from();

        if(response.port != route_server->second) {
            cout<<"The main server has received a response from an unexpected Server with port "<<response.port<<".\n";
            child.send_info(ROOM_NOT_FOUND);
        } else {
            string response_code;
            istringstream sstream {response.msg};
            getline(sstream, response_code);

            // if a successful reservation is made, update the room status
            if(response_code == ROOM_AVAILABLE) {
                cout<<"The main server received the response and the updated room status from Server "<<server_name<<" using UDP over port "<<serverM_backend<<".\n";

                int status;
                if(sstream >> status) room_status[room].second = status;
                else room_status[room].second = 0;
                cout<<"The room status of Room "<<room<<" has been updated.\n";

                child.send_info(response_code);
            } else {
                cout<<"The main server received the response from Server "<<server_name<<" using UDP over port "<<serverM_backend<<".\n";

                if(response_code != "") {
                    child.send_info(response_code);
                } else {
                    cout<<"The backend Server "<<server_name<<" has sent an empty response.\n";
                    child.send_info(ROOM_NOT_FOUND);
                }
            }
        }
    }

    cout<<"The main server sent the reservation result to the client.\n";
}


// accept availability and reservation requests from the client and respond appropriately
void accept_request(Socket& child, Socket& server_sock, const map<char, int>& router, unordered_map<string, pair<int, int>>& room_status, const bool member, const string& username, bool& open) {
    string request = child.recv_info();

    string request_type, room;
    istringstream sstream {request};

    // an empty input indicates a broken connection
    if(!getline(sstream, request_type)) {
        cout<<"The client with port "<<child.connected_port<<" has closed the connection.\n";
        open = false;
        return;
    }

    if(!getline(sstream, room)) {
        cout<<"The main server received a request with a missing room using TCP over port "<<serverM_client<<".\n";
        child.send_info(ROOM_EMPTY);
        return;
    }

    if(request_type == AVAILABILITY_REQUEST) {
        cout<<"The main server has received the availability request on Room "<<room<<" from "<<username<<" using TCP over port "<<serverM_client<<".\n";
        availability_request(child, server_sock, router, request, room);
    } else if(request_type == RESERVATION_REQUEST) {
        cout<<"The main server has received the reservation request on Room "<<room<<" from "<<username<<" using TCP over port "<<serverM_client<<".\n";
        create_reservation(child, server_sock, router, room_status, request, room, member, username);
    } else {
        cout<<"The main server received an invalid request type using TCP over port "<<serverM_client<<".\n";
        child.send_info(INVALID_REQUEST);
    }
}


// the main server is responsible for acting as an intermediary between the client and the backend servers
// it satisfies client authentication requests by reading user information from a file
// it satisfies client availability and reservation requests by querying the backend servers
int main() {
    constexpr bool debug = false;

    // collection of backend servers, mapping their port to their names
    const map<int, char> backend {{serverS, 'S'}, {serverD, 'D'}, {serverU, 'U'}};

    // router is a map between server names and their corresponding ports
    map<char, int> router {};
    for(const pair<int, char>& b : backend) router[b.second] = b.first;

    try {
        // create and bind the backend facing UDP socket
        Socket server_sock {-1, SOCK_DGRAM, serverM_backend, debug};
        server_sock.bind_socket(serverM_backend);

        // create and bind the client facing TCP socket
        Socket client_sock {-1, SOCK_STREAM, serverM_client, debug};
        client_sock.bind_socket(serverM_client);

        cout<<"The main server is up and running.\n";

        // room_status is a hashmap, mapping each room to its corresponding backend server and its count
        unordered_map<string, pair<int, int>> room_status = get_room_status(server_sock, backend);

        // user_info is a hashmap between usernames and corresponding passwords
        unordered_map<string, string> user_info = get_user_info(user_filename);

        client_sock.listen_socket();
        client_sock.reap_dead_processes();

        while(true) {
            Socket child = client_sock.accept_socket();

            // fork the process to allow the parent socket to continue listening and accepting connections
            if(!fork()) {
                // parent socket is no longer required
                client_sock.close_socket();

                bool open = true;
                bool member = false;
                string username;

                // accept authentication requests until the client is successfully authenticated,
                // or until the connection is closed
                bool success = false;
                while(open && !success) {
                    success = authenticate(child, user_info, member, username, open);
                }

                // accept availability and reservation requests until the connection is closed
                while(open) {
                    accept_request(child, server_sock, router, room_status, member, username, open);
                }

                break;
            }
        }

        return 0;

    } catch(socket_exception& se) {
        cout<<se.what()<<endl;
        return 1;
    }
}
