#include <string>
using namespace std;

// interface function for different backend servers
int run_backend(const char server_name, const int sock_port, const string& filename);
