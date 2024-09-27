#include <vector>
#include <sstream>
#include <iomanip>

#include "iostream"
#include "encrypt.h"

using namespace std;


// string representation of a char byte
string ctob(unsigned char c) {
    string s {""};
    for(int i = 7; i >= 0; i--) s += to_string(((c >> i) & 1));
    return s;
}


// string representation of a collection of char bytes
string stob(const vector<unsigned char>& bytes) {
    string out {""};

    int counter = 0;
    for(unsigned char c : bytes) {
        out += ctob(c) + ' ';

        counter++;
        if(counter == 8) {
            out += '\n';
            counter = 0;
        }
    }

    return out;
}


// string representation of a 32 bit integer as a hexadecimal number
string hex_string(uint32_t i) {
    ostringstream stream;
    stream<<setfill('0')<<setw(8)<<hex<<i;

    return stream.str();
}


// convert a 32 bit integer to little endian hexadecimal format
uint32_t lil_endian_hex(uint32_t n) {
    uint32_t lil = 0;
    for(int i = 0; i < 4; i++) {
        lil |= ((n >> (8*i)) & 0xff) << (8*(3-i));
    }

    return lil;
}


// create the message array by dividing a 512 bit chunk into 16 parts,
// and interpret each part to be in little endian format
void make_message(uint32_t* message, vector<unsigned char>::iterator& msg_itr) {
    for(int i = 0; i < 16; i++) {
        message[i] = 0;
        for(int j = 0; j < 4; j++) {
            message[i] |= (*(msg_itr++) << (8*j));
        }
    }
}


// pad the collection of bits to a multiple of 512
void pad(vector<unsigned char>& bytes) {
    uint64_t len = bytes.size() * 8;

    // append 1 to the collection
    bytes.push_back(0x80);

    // append 0s until the collection size becomes 448 bits (mod 512)
    while(bytes.size() % 64 != 56) bytes.push_back(0x00);

    // append the size of original message in little endian format as 64 bits
    for(int i = 0; i < 8; i++) bytes.push_back((len >> (8*i)) & 0xff);
}


// leftrotate F by i number of bits
uint32_t leftrotate(uint32_t F, unsigned int i) {
    return ((F << i) | (F >> (32 - i)));
}


// md5 hash function
// Impementation borrowed from: https://en.wikipedia.org/wiki/MD5
//                              https://www.ietf.org/rfc/rfc1321.txt
string md5_hash(vector<unsigned char>& bytes) {
    constexpr unsigned int s[64] { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                   5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                                   4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                   6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

    constexpr uint32_t K[64] { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                               0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                               0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                               0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                               0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                               0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                               0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                               0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                               0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                               0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                               0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                               0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                               0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                               0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                               0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                               0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

    // these variables are modified using the message and are combined to form the hash
    uint32_t a0 = 0x67452301;
    uint32_t b0 = 0xefcdab89;
    uint32_t c0 = 0x98badcfe;
    uint32_t d0 = 0x10325476;

    vector<unsigned char>::iterator msg_itr = bytes.begin();

    // process the message in chunks of 512 bits
    while(msg_itr != bytes.end()) {
        uint32_t message[16];
        make_message(message, msg_itr);

        // for(int i = 0; i < 16; i++) cout<<"0x"<<hex_string(message[i])<<endl;
        // cout<<endl;

        uint32_t A = a0;
        uint32_t B = b0;
        uint32_t C = c0;
        uint32_t D = d0;

        // process each chunk in 64 rounds
        for(int i = 0; i < 64; i++) {
            uint32_t F;
            unsigned int g;

            if(i < 16) {
                F = (B & C) | ((~B) & D);
                g = i;
            } else if(i >= 16 && i < 32) {
                F = (D & B) | ((~D) & C);
                g = (5*i + 1) % 16;
            } else if(i >= 32 && i < 48) {
                F = B ^ C ^ D;
                g = (3*i + 5) % 16;
            } else if(i >= 48 && i < 64) {
                F = C ^ (B | (~D));
                g = (7*i) % 16;
            }

            F = F + A + K[i] + message[g];

            A = D;
            D = C;
            C = B;
            B = B + leftrotate(F, s[i]);
        }

        a0 += A;
        b0 += B;
        c0 += C;
        d0 += D;
    }

    // combine the initial variables in little endian format to form the output hash string
    string out = "0x" + 
                  hex_string(lil_endian_hex(a0)) +
                  hex_string(lil_endian_hex(b0)) +
                  hex_string(lil_endian_hex(c0)) +
                  hex_string(lil_endian_hex(d0));

    return out;
}


// encrypt implements the md5 hash function and returns a hex string representing the 128 bit hash
string encrypt(const string& in) {
    // convert the string to a vector of characters
    // each char represents a byte
    vector<unsigned char> bytes {in.begin(), in.end()};

    // pad the collection of bits so the size in bits becomes a multiple of 512
    pad(bytes);

    // cout<<stob(bytes)<<endl;

    string encrypted = md5_hash(bytes);
    return encrypted;
}

const string user_filename {"member_extra.txt"};
