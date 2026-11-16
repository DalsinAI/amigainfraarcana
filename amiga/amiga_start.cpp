// Infra Arcana for GFOS: the program's version string and its stack.
// MIT, Copyright (c) 2026 AmiCradle Team.

// libnix gives the program this much stack when it starts (map generation
// and path finding recurse).
extern "C" unsigned long __stack = 1024 * 1024;

extern "C" const char ia_amiga_version[] __attribute__((used)) =
    "$VER: InfraArcana 23.0 (16.11.2026)";
