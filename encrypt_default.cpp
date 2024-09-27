#include "encrypt.h"
using namespace std;

// default encryption method
string encrypt(const string& s) {
    string encrypted {s};

    // cycle individual characters by 3
    for(char& c : encrypted) {
        if(islower(c)) c = 'a' + ((c-'a'+3) % ('z'-'a'+1));
        if(isupper(c)) c = 'A' + ((c-'A'+3) % ('Z'-'A'+1));
        if(isdigit(c)) c = '0' + ((c-'0'+3) % ('9'-'0'+1));
    }

    return encrypted;
}

const string user_filename {"member.txt"};
