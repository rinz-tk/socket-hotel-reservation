#include <string>

// encrypt determines the method of user credential encryption
std::string encrypt(const std::string& s);

// user_filename is the file which contains the encrypted user credentials
// using the specified encryption method
extern const std::string user_filename;
