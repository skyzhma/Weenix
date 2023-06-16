#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void usage()
{
    fprintf(stderr, "usage: invalid arguments\n");
    exit(-1);
}

void readData(char *string, char *type, char *dataName ,void *address) {

    if (string == NULL || sscanf(string, type, address) == -1) {
        printf("malformed command, value for %s is not given\n", dataName);
        exit(-1);
    }
}

double convertTimeStampToDouble(struct timeval time)
{
    double mul = 1000;
    double diff = time.tv_sec * mul + time.tv_usec / mul;
    return diff;
}

double timeSubtract(struct timeval* start, struct timeval* from) {
    struct timeval tmp;
    timersub(from, start, &tmp);
    return convertTimeStampToDouble(tmp);
}

void printHelp(char *file, double data, int flag, int numEnter) {
    printf("%s", file);
    if (flag) {
        printf("%.6lf", data / flag);
    } else {
        printf("N/A");
    }
    for(int i = 0; i < numEnter; i++) {
        printf("\n");
    }

}