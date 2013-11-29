#pragma once

extern int optind;
extern char optopt;
extern char* optarg;
char getopt(int argc, char** argv, const char* opts);
