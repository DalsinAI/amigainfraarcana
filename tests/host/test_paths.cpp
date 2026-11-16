// Host tests of the port's changes to src/paths.cpp: the game's data paths
// have no "//" (on AmigaDOS that is the parent drawer), and the Amiga build
// keeps user data in the drawer's User directory, made when missing.
// Built twice by run.sh: as upstream builds it, and with -D__amigaos__.
// MIT, Copyright (c) 2026 AmiCradle Team.
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "paths.hpp"

namespace io
{
std::string sdl_pref_dir()
{
    return "pref_dir/";
}
}  // namespace io

static int s_fails = 0;

static void check(bool ok, const std::string& what)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());

    if (!ok) {
        ++s_fails;
    }
}

static void check_eq(const std::string& got, const std::string& want, const std::string& what)
{
    check(got == want, what + ": '" + got + "'" + (got == want ? "" : " (want '" + want + "')"));
}

static bool is_dir(const char* p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

int main(int argc, char** argv)
{
    if (argc < 2 || chdir(argv[1]) != 0) {
        std::printf("FAIL usage: test_paths SCRATCH_DIR\n");
        return 1;
    }

    check_eq(paths::gfx_dir(), "gfx/", "gfx_dir");
    check_eq(paths::fonts_dir(), "gfx/fonts/", "fonts_dir");
    check_eq(paths::tiles_dir(), "gfx/tiles/20x20/", "tiles_dir");
    check_eq(paths::images_dir(), "gfx/images/", "images_dir");
    check_eq(paths::logo_img_path(), "gfx/images/main_menu_logo.png", "logo_img_path");
    check_eq(paths::audio_dir(), "audio/", "audio_dir");
    check_eq(paths::data_dir(), "data/", "data_dir");
    check_eq(paths::messages_dir(), "data/messages/", "messages_dir");

    for (const std::string& p : {paths::fonts_dir(), paths::tiles_dir(), paths::images_dir(),
                                 paths::logo_img_path(), paths::messages_dir()}) {
        check(p.find("//") == std::string::npos, "no // in " + p);
    }

    paths::init();

#ifdef __amigaos__
    check_eq(paths::user_dir(), "User/", "Amiga user_dir");
    check(is_dir("User"), "Amiga: User/ made");
    check(paths::pending_error_messages().empty(), "Amiga: no error message");
    check_eq(paths::save_file_path(), "User/save", "Amiga save_file_path");
    check_eq(paths::config_file_path(), "User/config", "Amiga config_file_path");
    check_eq(paths::highscores_file_path(), "User/highscores", "Amiga highscores_file_path");
    check(access("User/test.tmp", F_OK) != 0, "Amiga: test file removed");
    check(!is_dir("pref_dir"), "Amiga: SDL pref dir not used");

    // A drawer that cannot be written: the user is told, the path is kept.
    if (getuid() != 0) {
        chmod("User", 0555);
        paths::init();
        check_eq(paths::user_dir(), "User/", "Amiga read-only: user_dir kept");
        check(!paths::pending_error_messages().empty(), "Amiga read-only: error message");
        chmod("User", 0755);
    }
#else
    check_eq(paths::user_dir(), "pref_dir/", "upstream: SDL pref dir");
    check(!is_dir("User"), "upstream: no User/");
#endif

    std::printf("%s (%d failed)\n", s_fails ? "FAIL" : "PASS", s_fails);

    return s_fails ? 1 : 0;
}
