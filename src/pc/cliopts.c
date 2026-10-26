#include "cliopts.h"

#include "../sm64ap.h"
#include "configfile.h"
#include "cheats.h"
#include "pc_main.h"
#include "platform.h"
#include "macros.h"

#include <strings.h>
#include <stdlib.h>
#define __NO_MINGW_LFS //Mysterious error in MinGW.org stdio.h
#include <stdio.h>
#include <string.h>

struct PCCLIOptions gCLIOpts;

static void print_help(void) {
    printf("Super Mario 64 PC Port\n");
    printf("%-20s\tEnables the cheat menu.\n", "--cheats");
    printf("%-20s\tSaves the configuration file as CONFIGNAME.\n", "--configfile CONFIGNAME");
    printf("%-20s\tSets additional data directory name (only 'res' is used by default).\n", "--gamedir DIRNAME");
    printf("%-20s\tOverrides the default save/config path ('!' expands to executable path).\n", "--savepath SAVEPATH");
    printf("%-20s\tStarts the game in full screen mode.\n", "--fullscreen");
    printf("%-20s\tSkips the Peach and Castle intro when starting a new game.\n", "--skip-intro");
    printf("%-20s\tStarts the game in windowed mode.\n", "--windowed");
}

static inline int arg_string(const char *name, const char *value, char *target) {
    const unsigned int arglen = strlen(value);
    if (arglen >= SYS_MAX_PATH) {
        fprintf(stderr, "Supplied value for `%s` is too long.\n", name);
        return 0;
    }
    strncpy(target, value, arglen);
    target[arglen] = '\0';
    return 1;
}

static inline int arg_uint(UNUSED const char *name, const char *value, unsigned int *target) {
    const unsigned long int v = strtoul(value, NULL, 0);
    *target = v;
    return 1;
}

void parse_cli_opts(int argc, char* argv[]) {
    // Initialize options with false values.
    memset(&gCLIOpts, 0, sizeof(gCLIOpts));

    int idx_ip = 0, idx_name = 0, idx_passwd = 0, idx_file = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--skip-intro") == 0) // Skip Peach Intro
            gCLIOpts.SkipIntro = 1;

        else if (strcmp(argv[i], "--fullscreen") == 0) // Open game in fullscreen
            gCLIOpts.FullScreen = 1;

        else if (strcmp(argv[i], "--windowed") == 0) // Open game in windowed mode
            gCLIOpts.FullScreen = 2;

        else if (strcmp(argv[i], "--cheats") == 0) // Enable cheats menu
            Cheats.EnableCheats = true;

        else if (strcmp(argv[i], "--poolsize") == 0) // Main pool size
            arg_uint("--poolsize", argv[++i], &gCLIOpts.PoolSize);

        else if (strcmp(argv[i], "--configfile") == 0 && (i + 1) < argc)
            arg_string("--configfile", argv[++i], gCLIOpts.ConfigFile);

        else if (strcmp(argv[i], "--gamedir") == 0 && (i + 1) < argc)
            arg_string("--gamedir", argv[++i], gCLIOpts.GameDir);

        else if (strcmp(argv[i], "--savepath") == 0 && (i + 1) < argc)
            arg_string("--savepath", argv[++i], gCLIOpts.SavePath);

        else if (strcmp(argv[i], "--sm64ap_ip") == 0 && (i + 1) < argc) idx_ip = ++i;
        else if (strcmp(argv[i], "--sm64ap_name") == 0 && (i + 1) < argc) idx_name = ++i;
        else if (strcmp(argv[i], "--sm64ap_passwd") == 0 && (i + 1) < argc) idx_passwd = ++i;

        else if (strcmp(argv[i], "--sm64ap_file") == 0 && (i + 1) < argc) idx_file = ++i;

        // Print help
        else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            game_exit();
        }
    }
    // Without either of these the connection saved in the chosen save file is used (see Connect on the file select).
    if (idx_name != 0) {
        SM64AP_InitMW(idx_ip == 0 ? "" : argv[idx_ip], argv[idx_name], idx_passwd == 0 ? "" : argv[idx_passwd]);
    } else if (idx_file != 0) {
        SM64AP_InitSP(argv[idx_file]);
    }
}
