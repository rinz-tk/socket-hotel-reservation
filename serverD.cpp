#include "backend.h"
#include "constants.h"

// run serverD program
int main() {
    const string filename = "double.txt";

    return run_backend('D', socket_constants::serverD, filename);
}
