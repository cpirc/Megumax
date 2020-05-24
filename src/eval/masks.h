#ifndef MEGUMAX_EVAL_MASKS_H
#define MEGUMAX_EVAL_MASKS_H

#include <array>

#include <libchess/Bitboard.h>
#include <libchess/Color.h>
#include <libchess/Lookups.h>
#include <libchess/Square.h>

namespace megumax {

namespace init {

inline std::array<std::array<libchess::Bitboard, 64>, 2> passed_pawn_masks() {
    std::array<std::array<libchess::Bitboard, 64>, 2> masks{};
    for (libchess::Square sq : libchess::constants::SQUARES) {
        masks[libchess::constants::WHITE][sq] = libchess::Bitboard{};
        masks[libchess::constants::BLACK][sq.flipped()] = libchess::Bitboard{};
        if (sq.file() != libchess::constants::FILE_A) {
            masks[libchess::constants::WHITE][sq] |=
                libchess::lookups::north(sq - 1) | libchess::lookups::north(sq);
            masks[libchess::constants::BLACK][sq.flipped()] |=
                libchess::lookups::south(sq.flipped() - 1) | libchess::lookups::south(sq.flipped());
        }
        if (sq.file() != libchess::constants::FILE_H) {
            masks[libchess::constants::WHITE][sq] |=
                libchess::lookups::north(sq + 1) | libchess::lookups::north(sq);
            masks[libchess::constants::BLACK][sq.flipped()] |=
                libchess::lookups::south(sq.flipped() + 1) | libchess::lookups::south(sq.flipped());
        }
    }
    return masks;
}

}  // namespace init

static std::array<std::array<libchess::Bitboard, 64>, 2> PASSED_PAWN_MASK =
    init::passed_pawn_masks();

inline libchess::Bitboard passed_pawn_mask(libchess::Square square, libchess::Color color) {
    return PASSED_PAWN_MASK[color][square];
}

}  // namespace megumax

#endif  // MEGUMAX_EVAL_MASKS_H
