#include "backend.h"
#include "constants.h"

// run serverU program
int main() {
    const string filename = "suite.txt";

    return run_backend('U', socket_constants::serverU, filename);
}
