#ifndef NOLIBC
#include <sys/mount.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

const char* USAGE_MSG = "Usage: mount [options] {source} {target} {fstype}";

/*
int mount(const char *source, const char *target,
            const char *filesystemtype, unsigned long mountflags,
            const void *_Nullable data);
*/

int main (int argc, char** argv) {
    unsigned long mountflags = 0;
    char* source = NULL;
    char* target = NULL;
    char* fstype = NULL;

    if (argc <= 1) {
        puts(USAGE_MSG);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (strlen(arg) == 0)
            continue;

        if (arg[0] != '-') { // If the arg is not an option...
            if (source == NULL) {
                source = arg;
                continue;
            }
            else if (target == NULL) {
                target = arg;
                continue;
            }
            else if (fstype == NULL) {
                fstype = arg;
                continue;
            }
            else {
                puts(USAGE_MSG);
                return EXIT_FAILURE;
            }
        }

        if (strcmp(arg, "--remount") == 0) {
            mountflags |= MS_REMOUNT;
        }
        else if (strcmp(arg, "--bind") == 0) {
            mountflags |= MS_BIND;
        }
        else if (strcmp(arg, "--shared") == 0) {
            mountflags |= MS_SHARED;
        }
        else if (strcmp(arg, "--private") == 0) {
            mountflags |= MS_PRIVATE;
        }
        else if (strcmp(arg, "--slave") == 0) {
            mountflags |= MS_SLAVE;
        }
        else if (strcmp(arg, "--unbindable") == 0) {
            mountflags |= MS_UNBINDABLE;
        }
        else if (strcmp(arg, "--move") == 0) {
            mountflags |= MS_MOVE;
        }
        else if (strcmp(arg, "--dirsync") == 0) {
            mountflags |= MS_DIRSYNC;
        }
        else if (strcmp(arg, "--lazytime") == 0) {
            mountflags |= MS_LAZYTIME;
        }
        else if (strcmp(arg, "--mandlock") == 0) {
            mountflags |= MS_MANDLOCK;
        }
        else if (strcmp(arg, "--noatime") == 0) {
            mountflags |= MS_NOATIME;
        }
        else if (strcmp(arg, "--nodev") == 0) {
            mountflags |= MS_NODEV;
        }
        else if (strcmp(arg, "--nodiratime") == 0) {
            mountflags |= MS_NODIRATIME;
        }
        else if (strcmp(arg, "--noexec") == 0) {
            mountflags |= MS_NOEXEC;
        }
        else if (strcmp(arg, "--nosuid") == 0) {
            mountflags |= MS_NOSUID;
        }
        else if (strcmp(arg, "--rdonly") == 0) {
            mountflags |= MS_RDONLY;
        }
        else if (strcmp(arg, "--rec") == 0) {
            mountflags |= MS_REC;
        }
        else if (strcmp(arg, "--relatime") == 0) {
            mountflags |= MS_RELATIME;
        }
        else if (strcmp(arg, "--silent") == 0) {
            mountflags |= MS_SILENT;
        }
        else if (strcmp(arg, "--strictatime") == 0) {
            mountflags |= MS_STRICTATIME;
        }
        else if (strcmp(arg, "--synchronous") == 0) {
            mountflags |= MS_SYNCHRONOUS;
        }
        // This doesn't seem to be present in glibc. Fairly newer feature, I think.
        // else if (strcmp(arg, "--nosymfollow") == 0) {
        //     mountflags |= MS;
        // }
        else {
            puts(USAGE_MSG);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}