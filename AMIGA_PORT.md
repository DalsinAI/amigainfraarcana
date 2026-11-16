Infra Arcana for GFOS
=====================

A port of Infra Arcana, Martin Törnqvist's roguelike set in the world of
H. P. Lovecraft, from https://gitlab.com/martin-tornqvist/ia at commit
2693b3ca (version 23.0.0), for GFOS: AmigaOS 3.2 with GFOpenUp, on a
68040 or 68060 with an FPU, on GFOpenUp's SDL 2.

You are an investigator of the 1920s going down into the Cult of Starry
Wisdom's catacombs to find the Shining Trapezohedron. Every game is a new
dungeon; death is for good.

Status: see the pull request (stove build and lab).


What you need
-------------

- GFOS: AmigaOS 3.2 with GFOpenUp, which installs LIBS:GFOpenGPU/
  SDL2.module, SDL2_image.module and SDL2_mixer.module.
- A 68040 or 68060 with an FPU (not an LC040 or EC040), an RTG screen of
  800 x 600 or more (or the AmiCradle lab), and about 32 MB of free
  memory.
- About 45 MB on the disk.


How to play
-----------

Run Install-InfraArcana (or copy the InfraArcana drawer where you want
it), then double-click InfraArcana, or from a Shell:

    InfraArcana

from anywhere: the game makes its own drawer its current directory while
it runs. The keys are in the drawer's Controls file and the whole manual
in manual.txt (? in the game). Your options, high scores, saved game and
game summaries are kept in the drawer's User drawer; Install-InfraArcana
keeps it when it updates the game.


Building
--------

    make -f amiga/Makefile.amigaos -j4 STOVE=/path/to/os32-gcc16-open6

or, in Cradle, stove_build with the os32-gcc16-open6 stove and target
package (package/build-amiga.sh), which also packs
InfraArcana-VERSION-68040.lha.

    tests/host/run.sh

runs the host tests of the port's changes (no SDL needed).


What was changed
----------------

- src/paths.cpp: the data paths no longer have "//" in them
  ("gfx//fonts/"): on AmigaDOS that is the parent drawer, elsewhere it is
  the same as "/". The Amiga build keeps user data in User/ in the game's
  drawer (made when missing) instead of user_data.ini's path or
  SDL_GetPrefPath, and tests that it can write there without
  std::filesystem. tests/host/test_paths.cpp checks both builds.
- src/config.cpp: the Amiga build lists the font pictures through
  dos.library (amiga/amiga_dir.cpp) instead of
  std::filesystem::directory_iterator, which libnix does not back.
- src/main.cpp: the Amiga build makes PROGDIR: its current directory
  first (amiga_enter_progdir), so gfx/, audio/, data/ and User/ are found
  when it is started from a Shell elsewhere; the Shell's directory comes
  back when the game exits.
- amiga/lgamma.c: lgamma(), which libstdc++'s std::binomial_distribution
  (src/random.cpp) needs and libnix's maths library lacks (Lanczos, about
  15 digits); tests/host/test_lgamma.c checks it against the host's.
- amiga/amiga_start.cpp: a 1 MB stack and the $VER string.
- Nothing else in the game is changed; tinyxml2 and mINI are upstream's
  bundled copies.

Designed, tested and delivered by The AmiCradle Team.
