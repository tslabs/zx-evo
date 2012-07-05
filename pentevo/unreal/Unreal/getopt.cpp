#include "std.h"
#include "getopt.h"
#include "util.h"

int optind = 0;
char optopt;
char* optarg;

char getopt(int argc, char** argv, const char* opts)
{
    static int sp = 1;
    if (sp == 1) {
        if (optind >= argc || (argv[optind][0] != '-' && argv[optind][0] != '/') || !argv[optind][1])
            return EOF;
        else if (!strcmp(argv[optind], "--")) {
            optind++;
            return EOF;
        }
    }
    char *cp;
    char c = optopt = argv[optind][sp];
    if (c == ':' || !(cp = strchr((char*)opts, c))) {
        errmsg("illegal option /%c", c);
        if (!argv[optind][++sp]) {
            optind++;
            sp = 1;
        }
        return '?';
    }
    if (*++cp == ':') {
        if (argv[optind][sp+1])
            optarg = &argv[optind++][sp+1];
        else if (++optind >= argc) {
            errexit("/%c requires an argument", c);
            sp = 1;
            return '?';
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if (!argv[optind][++sp]) {
            sp = 1;
            optind++;
        }
        optarg = 0;
    }
    return c;
}
