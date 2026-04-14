#ifndef VERSION_H
#define VERSION_H

/*
========================================
        GBCEE VERSIONING SYSTEM
========================================

Semantic Versioning Format:
MAJOR.MINOR.PATCH

MAJOR → Breaking changes / architecture overhaul
MINOR → New features (MMU, PPU, timing, etc.)
PATCH → Bug fixes / small improvements

Example:
0.3.5 → Functional emulator, patch fixes
*/

/* ================================
   CORE VERSION COMPONENTS
================================ */

#define GBCEE_VERSION_MAJOR 0
#define GBCEE_VERSION_MINOR 3
#define GBCEE_VERSION_PATCH 0

/* ================================
   BUILD METADATA (optional)
================================ */

/* Manually set or inject via CMake/Git later */
#define GBCEE_VERSION_TAG    "dev"      // e.g. "alpha", "beta", "release"
#define GBCEE_BUILD_TYPE     "debug"    // debug / release

/* ================================
   STRING HELPERS
================================ */

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/* ================================
   FULL VERSION STRING
================================ */

#define GBCEE_VERSION_STRING \
    STR(GBCEE_VERSION_MAJOR) "." \
    STR(GBCEE_VERSION_MINOR) "." \
    STR(GBCEE_VERSION_PATCH)

/* Optional extended version */
#define GBCEE_FULL_VERSION \
    GBCEE_VERSION_STRING "-" GBCEE_VERSION_TAG

/* ================================
   VERSION STRUCT (OPTIONAL)
================================ */

typedef struct {
    int major;
    int minor;
    int patch;
    const char* tag;
} gbcee_version_t;

/* ================================
   ACCESS MACRO
================================ */

#define GBCEE_VERSION_INIT { \
    GBCEE_VERSION_MAJOR, \
    GBCEE_VERSION_MINOR, \
    GBCEE_VERSION_PATCH, \
    GBCEE_VERSION_TAG \
}

/* ================================
   PRINT HELPERS
================================ */

#define GBCEE_PRINT_VERSION() \
    printf("GBCee Version: %s (%s)\n", \
        GBCEE_FULL_VERSION, \
        GBCEE_BUILD_TYPE)

#endif /* VERSION_H */