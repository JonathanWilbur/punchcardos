#ifndef NOLIBC
#include <stdio.h>
#endif

int main (int argc, char** argv) {
    return printf("\e[1;1H\e[2J") > 0;
}