#include <cctype>
#include <iostream>
#include <sys/socket.h>

#include "socket.h"
#include "encrypt.h"
#include "constants.h"

using namespace std;
using namespace socket_constants;


// input, send and receive authentication information
bool authenticate(Socket& sock, string& username, bool& open) {
    bool member = false;
    string password;

    cout<<"Please enter the username: ";

    getline(cin, username);
    if(username == "") {
        cout<<"Username is required.\n";
        return false;
    }

    cout<<"Please enter the password: (Press \"Enter\" to skip) ";

    getline(cin, password);
    if(password != "") member = true;
    else member = false;

    string e_username = encrypt(username);
    string e_password {""};
    if(member) e_password = encrypt(password);

    sock.send_info(e_username + '\n' + e_password);

    if(member) cout<<username<<" sent an authentication request to the main server.\n";
    else cout<<username<<" sent a guest request to the main server using TCP over port "<<sock.bound_port()<<".\n";

    string result = sock.recv_info();

    bool success = false;
    if(result == VALID_MEMBER) {
        success = true;
        cout<<"Welcome member "<<username<<"!\n";
    } else if (result == VALID_GUEST) {
        success = true;
        cout<<"Welcome guest "<<username<<"!\n";
    } else if (result == INVALID_PASSWORD) {
        cout<<"Failed login: Password does not match.\n";
    } else if (result == INVALID_USER) {
        cout<<"Failed login: Username does not exist.\n";
    } else if (result == CLOSED_CONNECTION) {
        cout<<"The main server has closed the connection.\n";
        open = false;
    } else {
        cout<<"Failed login: Invalid server response.\n";
    }

    return success;
}


// send and receive availability information
void check_availability(Socket& sock, const string& room, const string& username, bool& open) {
    sock.send_info(AVAILABILITY_REQUEST + ('\n' + room));
    cout<<username<<" sent an availability request to the main server.\n";

    string result = sock.recv_info();
    cout<<"The client received the response from the main server using TCP over port "<<sock.bound_port()<<".\n";

    if(result == ROOM_AVAILABLE) {
        cout<<"The requested room is available.\n";
    } else if(result == ROOM_NOT_AVAILABLE) {
        cout<<"The requested room is not available.\n";
    } else if(result == ROOM_NOT_FOUND) {
        cout<<"Not able to find the room layout.\n";
    } else if(result == REQUEST_EMPTY) {
        cout<<"Not able to detect a request type.\n";
    } else if(result == ROOM_EMPTY) {
        cout<<"Not able to detect a requested room.\n";
    } else if(result == INVALID_REQUEST) {
        cout<<"The main server detected an invalid request.\n";
    } else if(result == CLOSED_CONNECTION) {
        cout<<"The main server has closed the connection.\n";
        open = false;
    } else {
        cout<<"Failed to retrieve availability: Invalid server response.\n";
    }

    cout<<endl;
}


// send and receive reservation information
void create_reservation(Socket& sock, const string& room, const string& username, bool& open) {
    sock.send_info(RESERVATION_REQUEST + ('\n' + room));
    cout<<username<<" sent a reservation request to the main server.\n";

    string result = sock.recv_info();

    if(result == USER_NOT_MEMBER) {
        cout<<"Permission denied: Guest cannot make a reservation.\n";
    } else {
        cout<<"The client received the response from the main server using TCP over port "<<sock.bound_port()<<".\n";

        if(result == ROOM_AVAILABLE) {
            cout<<"Congratulation! The reservation for Room "<<room<<" has been made.\n";
        } else if(result == ROOM_NOT_AVAILABLE) {
            cout<<"Sorry! The requested room is not available.\n";
        } else if(result == ROOM_NOT_FOUND) {
            cout<<"Oops! Not able to find the room.\n";
        } else if(result == REQUEST_EMPTY) {
            cout<<"Not able to detect a request type.\n";
        } else if(result == ROOM_EMPTY) {
            cout<<"Not able to detect a requested room.\n";
        } else if(result == INVALID_REQUEST) {
            cout<<"The main server detected an invalid request.\n";
        } else if(result == CLOSED_CONNECTION) {
            cout<<"The main server has closed the connection.\n";
            open = false;
        } else {
            cout<<"Failed to make reservation: Invalid server response.\n";
        }
    }

    cout<<endl;
}


// prompt the user to input a room
string input_room() {
    string room;
    while(true) {
        cout<<"Please enter the room code: ";
        getline(cin, room);

        // disallow empty room strings
        if(room == "") cout<<"Room is required.\n\n";
        else break;
    }

    return room;
}


// prompt the user to enter a request type
string input_request() {
    string request;
    cout<<"Would you like to search for the availability or make a reservation? ";
    cout<<"(Enter \"Availability\" to search for the availability or Enter \"Reservation\" to make a reservation ): ";

    getline(cin, request);
    return request;
}


// client program receives requests from the user and communicates with the main server to satisfy these requests
int main() {
    constexpr bool debug = false;

    try {
        // create the client TCP socket
        Socket sock {-1, SOCK_STREAM, -1, debug};
        cout<<"Client is up and running.\n";

        sock.connect_socket(serverM_client);

        bool open = true;
        string username {};

        // repeatedly prompt until the user is succesfully authenticated, or if the connection is closed
        bool success = false;
        while(open && !success) {
            success = authenticate(sock, username, open);
        }

        // repeatedly prompt for requests until the connection is closed
        while(open) {
            string room = input_room();
            string request = input_request();

            if(request == "Availability") check_availability(sock, room, username, open);
            else if(request == "Reservation") create_reservation(sock, room, username, open);
            else cout<<"Invalid request entered.\n\n";

            if(open) cout<<"-----Start a new request-----\n";
        }

        return 0;

    } catch(socket_exception& se) {
        cout<<se.what()<<endl;
        return 1;
    }
}
