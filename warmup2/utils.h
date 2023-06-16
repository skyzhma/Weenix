#include<sys/time.h>
struct RunningAverage
{
    double data;
    double times;
};

void usage();
double convertTimeStampToDouble(struct timeval);
double timeSubtract(struct timeval*, struct timeval*);
void printHelp(char*, double, int, int);
void readData(char*, char*, char*, void*);