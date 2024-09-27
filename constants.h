namespace socket_constants {
    // port numbers
    constexpr int serverS = 41626;
    constexpr int serverD = 42626;
    constexpr int serverU = 43626;
    constexpr int serverM_backend = 44626;
    constexpr int serverM_client = 45626;

    // a received empty string notifies of a closed TCP connection
    constexpr char CLOSED_CONNECTION[] = "";

    // book status send complete
    constexpr char FINISH_STATUS[] = "0";

    // authorization codes
    constexpr char VALID_MEMBER[] = "0";
    constexpr char VALID_GUEST[] = "1";
    constexpr char INVALID_PASSWORD[] = "2";
    constexpr char INVALID_USER[] = "3";

    // request type codes
    constexpr char AVAILABILITY_REQUEST[] = "A";
    constexpr char RESERVATION_REQUEST[] = "R";

    // availability and reservation codes
    constexpr char ROOM_AVAILABLE[] = "0";
    constexpr char ROOM_NOT_AVAILABLE[] = "1";
    constexpr char ROOM_NOT_FOUND[] = "2";
    constexpr char USER_NOT_MEMBER[] = "3";
    constexpr char REQUEST_EMPTY[] = "4";
    constexpr char ROOM_EMPTY[] = "5";
    constexpr char INVALID_REQUEST[] = "6";
}
