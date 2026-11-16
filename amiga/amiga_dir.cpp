// The AmigaOS side of Infra Arcana for GFOS: see amiga_dir.hpp.
// MIT, Copyright (c) 2026 AmiCradle Team.
#include "amiga_dir.hpp"

#include <cstdlib>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

static BPTR s_progdir_lock = 0;
static BPTR s_old_dir = 0;
static bool s_entered = false;

static void amiga_leave_progdir()
{
    if (!s_entered) {
        return;
    }

    CurrentDir(s_old_dir);
    UnLock(s_progdir_lock);

    s_entered = false;
}

void amiga_enter_progdir()
{
    if (s_entered) {
        return;
    }

    s_progdir_lock = Lock((CONST_STRPTR) "PROGDIR:", SHARED_LOCK);

    if (!s_progdir_lock) {
        return;
    }

    s_old_dir = CurrentDir(s_progdir_lock);
    s_entered = true;

    atexit(amiga_leave_progdir);
}

std::vector<std::string> amiga_list_files(const std::string& dir)
{
    std::vector<std::string> names;

    std::string path = dir;

    // "gfx/fonts/" -> "gfx/fonts": Lock() is given the plain drawer name.
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }

    const BPTR lock = Lock((CONST_STRPTR)path.c_str(), SHARED_LOCK);

    if (!lock) {
        return names;
    }

    struct FileInfoBlock* const fib =
        (struct FileInfoBlock*)AllocDosObject(DOS_FIB, nullptr);

    if (fib) {
        if (Examine(lock, fib) && fib->fib_DirEntryType > 0) {
            while (ExNext(lock, fib)) {
                if (fib->fib_DirEntryType < 0) {
                    names.emplace_back((const char*)fib->fib_FileName);
                }
            }
        }

        FreeDosObject(DOS_FIB, fib);
    }

    UnLock(lock);

    return names;
}
