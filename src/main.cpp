#include "libchess/Position.h"

int main() {
    libchess::Position pos{libchess::constants::STARTPOS_FEN};
    pos.display();

    return 0;
}
