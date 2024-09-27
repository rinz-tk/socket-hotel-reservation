#include <fstream>
#include <iostream>

#include "encrypt.h"

using namespace std;

int main() {
    ifstream f {"member_unencrypted.txt"};

    string username, password;
    while(f.good() && getline(f, username, ',') && f.ignore() && getline(f, password, '\r')) {
        cout<<encrypt(username)<<", "<<encrypt(password)<<'\n';
        f.ignore();
    }

}
