//
// Created by TYTY on 2019-05-07 007.
//

#include "..\third_party\easyloggingpp\src\easylogging++.h"

#include "..\file.h"

INITIALIZE_EASYLOGGINGPP

const int PIECE_SIZE = 65536;

typedef std::uintmax_t mint;

int main(int argc, char* argv[]) {
    LOG(INFO) << "Test program";

    if (argc != 3) {
        LOG(ERROR) << "Usage: file source dest";
        return 1;
    }

    file::file_reader r(argv[1], PIECE_SIZE);

    file::file_writer w(argv[2], r.get_size());

    int read_size;
    mint offset;
    char buf[PIECE_SIZE];
    memset(buf, 0, PIECE_SIZE);

    do {
        read_size = r.read(buf, offset);

        if (read_size == 0) break;

        w.write(buf, read_size, offset);
        memset(buf, 0, PIECE_SIZE);
    } while (read_size == PIECE_SIZE);

    return 0;
}