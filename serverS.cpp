#include "backend.h"
#include "constants.h"

// run serverS program
int main() {
    const string filename = "single.txt";

    return run_backend('S', socket_constants::serverS, filename);
}
