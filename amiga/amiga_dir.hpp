// The AmigaOS side of Infra Arcana for GFOS: its current directory and
// drawer listing, in place of std::filesystem (libnix has none underneath).
// MIT, Copyright (c) 2026 AmiCradle Team.
#ifndef IA_AMIGA_DIR_HPP
#define IA_AMIGA_DIR_HPP

#include <string>
#include <vector>

// Makes PROGDIR: (the game's drawer) the current directory, so that the
// game's relative gfx/, audio/, data/ and User/ paths are found when it is
// started from a Shell anywhere. The Shell's own directory comes back when
// the game exits, however it exits.
void amiga_enter_progdir();

// The names of the plain files in a drawer (no directories), in the order
// dos.library gives them; empty when the drawer cannot be read.
std::vector<std::string> amiga_list_files(const std::string& dir);

#endif  // IA_AMIGA_DIR_HPP
