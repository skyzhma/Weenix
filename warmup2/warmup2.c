#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include "mylist.h"
#include "utils.h"

int n = 20;
double lambda = 1;
double mu = 0.35;
double r = 1.5;
int B = 10;
int P = 3;
char fileName[80];
double mul = 1000;

List *Q1;
List *Q2;
List *packets;
int serverIds[2];

struct timeval startTime;
struct timeval currentTime;
double totalEmulationTime;

pthread_t tockenArriveThreadId;
pthread_t packetArriveThreadId;
pthread_t serverThreadId[2];
pthread_t signalThreadId;
pthread_mutex_t m;
pthread_cond_t cv;
int signalDetect = 0;

sigset_t set;

int numPacketArrived = 0;
int numPacketServed = 0;
int numPacketDropped = 0;
int numPacketProduced = 0;
int numTokenDropped = 0;
int numTokenProduced = 0;

double packetInterArrivalTime = 0;
double packetServiceTime = 0;
double timeSpentInQ1 = 0;
double timeSpentInQ2 = 0;
double timeSpentInServers[2];
double averageTimeSpentInSystem = 0;
double averageTimeSquareSpentInSystem = 0;

void parseCommandLine(int argc, char *argv[]) {
    for(argc--, argv++; argc > 0; argc-=2, argv+=2) {
        if (strcmp(*argv, "-lambda") == 0) {
            readData(*(argv+1), "%lf", "lambda", &lambda);
        } else if (strcmp(*argv, "-mu") == 0) {
            readData(*(argv+1), "%lf", "mu", &mu);
        } else if (strcmp(*argv, "-r") == 0) {
            readData(*(argv+1), "%lf", "r", &r);
        } else if (strcmp(*argv, "-n") == 0) {
            readData(*(argv+1), "%d", "n", &n);
        } else if (strcmp(*argv, "-B") == 0) {
            readData(*(argv+1), "%d", "B", &B);
        } else if (strcmp(*argv, "-P") == 0) {
            readData(*(argv+1), "%d", "P", &P);
        } else if (strcmp(*argv, "-t") == 0) {
            readData(*(argv+1), "%s", "tsfile", fileName);
            if (access(fileName, 0) != -1) {

                struct stat path; 
                stat(fileName, &path);
                if(S_ISREG(path.st_mode) == 0) {
                    printf("input file %s is a directory\n", fileName);
                    exit(-1);
                }
                if (access(fileName, 2) == -1) {
                    printf("input file %s cannot be opened - access denies\n", fileName);
                    exit(-1);
                }
            } else {
                printf("input file %s does not exist\n", fileName);
                exit(-1);
            }


        } else {
            printf("malformed command, %s is not a valid command option\n", *argv);
            exit(-1);
        }
    }
    
}

void printParameters() {
    printf("Emulation Parameters:\n");
    printf("\tnumber to arrive = %d\n", n);
    if (strlen(fileName) == 0) {
        printf("\tlambda = %.2lf\n", lambda);
        printf("\tmu = %.2lf\n", mu);
    }
    printf("\tr = %.2lf\n", r);
    printf("\tB = %d\n", B);
    printf("\tP = %d\n", P);
    if (strlen(fileName) != 0) {
        printf("\ttsfile = %s\n", fileName);
    }
    printf("\n");
}

void printStatistics() {
    printf("\nStatistics:\n");

    printHelp("\taverage packet inter-arrival time = ", packetInterArrivalTime / mul, numPacketProduced, 1);
    printHelp("\taverage packet service time = ", packetServiceTime / mul, numPacketServed, 2);
    printf("\taverage number of packets in Q1 = %.6lf\n", timeSpentInQ1 / totalEmulationTime);
    printf("\taverage number of packets in Q2 = %.6lf\n", timeSpentInQ2 / totalEmulationTime);
    printf("\taverage number of packets in S1 = %.6lf\n", timeSpentInServers[0] / totalEmulationTime);
    printf("\taverage number of packets in S2 = %.6lf\n\n", timeSpentInServers[1] / totalEmulationTime);
    printf("\taverage time a packet spent in system = %.6lf\n", averageTimeSpentInSystem);
    printf("\tstand deviation for time spent in system = %.6lf\n\n", sqrt(
            (averageTimeSquareSpentInSystem - averageTimeSpentInSystem * averageTimeSpentInSystem)));
    printHelp("\ttoken drop probability = ", numTokenDropped, numTokenProduced, 1);
    printHelp("\tpacket drop probability = ", numPacketDropped, numPacketProduced, 1);
}

void initData() {

    Q1 = (List *)malloc(sizeof(List));
    Q2 = (List *)malloc(sizeof(List));
    packets = (List *)malloc(sizeof(List));
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cv, NULL);
    serverIds[0] = 1;
    serverIds[1] = 2;

    if (strlen(fileName) == 0) {
        for(int i = 0; i < n; i++) {
            
            Packet *newPacket = packetInit((int)((1/mu > 10 ? 10 : 1/mu)*mul),
                                            P,
                                            (int)((1/lambda > 10 ? 10 : 1/lambda)*mul),
                                            i+1);
            ListElem *elem = elemInit(newPacket);
            ListAppend(packets, elem);
        }
    } else {
        FILE *fp = fopen(fileName, "r");
        if(fscanf(fp, "%d", &n)){
            for(int i = 0; i < n; i++) {
                Packet *newPacket = packetInit(0,0,0,i+1);
                fscanf(fp, "%d", &(newPacket->interArrivalTime));
                fscanf(fp, "%d", &(newPacket->numTokenNeeded));
                fscanf(fp, "%d", &(newPacket->serviceTime));
                ListElem *elem = elemInit(newPacket);
                ListAppend(packets, elem);
            }
        } else {
            printf("malformed input - line 1 is not just a number\n");
            exit(-1);
        }
    }

}

void freeData() {
    free(Q1);
    free(Q2);
    free(packets);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cv);
}

void *tockenArrive(void *arg) {

    int numToken = 0;
    int tokenId = 0;
    Packet *tmp;

    pthread_cleanup_push((void *)pthread_mutex_unlock, &m);
    while (1) {
        usleep(1 / r * 1000000);
        pthread_mutex_lock(&m);
        gettimeofday(&currentTime, NULL);
        numTokenProduced++;
        tokenId++;
        if (numToken >= B) {
            printf("%011.3lfms: token%d arrives, dropped\n", timeSubtract(&startTime, &currentTime), tokenId);
            numTokenDropped++;

        } else {
            numToken++;
            printf("%011.3lfms: token%d arrives, token bucket now has %d token", timeSubtract(&startTime, &currentTime), tokenId, numToken);
            if (numToken > 1) { printf("s");}
            printf("\n");
        }
        
        while (!ListEmpty(Q2) && numPacketArrived + numPacketDropped != n) {
            pthread_cond_wait(&cv, &m);
        }

        while (!ListEmpty(Q1) && numToken >= listNextPacketTokenNeed(Q1)) {

            ListElem *elem = listNext(Q1);
            tmp = (elem->obj);

            numToken -= tmp->numTokenNeeded;

            gettimeofday(&currentTime, NULL);

            printf("%011.3lfms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n", 
                timeSubtract(&startTime, &currentTime),
                tmp->packetId, 
                timeSubtract(&(tmp->arriveQ1), &currentTime),
                numToken);
            
            timeSpentInQ1 += timeSubtract(&(tmp->arriveQ1), &currentTime);

            ListAppend(Q2, elem);
            gettimeofday(&currentTime, NULL);
            printf("%011.3lfms: p%d enters Q2\n",  timeSubtract(&startTime, &currentTime), tmp->packetId);

            tmp->arriveQ2 = currentTime;

            numPacketArrived++;
            pthread_cond_broadcast(&cv);
        }

        // printf("packet arrived %d, packet dropped %d\n", numPacketArrived, numPacketDropped);
        if (numPacketArrived + numPacketDropped == n) {
            pthread_mutex_unlock(&m);
            pthread_cond_broadcast(&cv);
            pthread_exit(NULL);
        }

        pthread_mutex_unlock(&m);
    }
    pthread_cleanup_pop(1);

}

void *packetArrive(void *arg) {

    struct timeval lastArrivalTime = startTime;
    double interArrivalTime = 0;

    pthread_cleanup_push((void *)pthread_mutex_unlock, &m);
    while (numPacketProduced < n) {

        ListElem *elem = listNext(packets);
        Packet *newPacket = (elem->obj);
        usleep(newPacket->interArrivalTime * mul);
        numPacketProduced++;

        pthread_mutex_lock(&m);
        gettimeofday(&currentTime, NULL);
        newPacket->arrives = currentTime;
        interArrivalTime = timeSubtract(&lastArrivalTime, &currentTime);
        lastArrivalTime = currentTime;
        // Store statistics data
        packetInterArrivalTime += interArrivalTime;

        if (newPacket->numTokenNeeded <= B) {
            printf("%011.3lfms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms\n", 
                timeSubtract(&startTime, &currentTime),
                numPacketProduced, newPacket->numTokenNeeded, 
                interArrivalTime);

            ListAppend(Q1, elem);

            gettimeofday(&currentTime, NULL);
            newPacket->arriveQ1 = currentTime;
            printf("%011.3lfms: p%d enters Q1\n", timeSubtract(&startTime, &currentTime), numPacketProduced);
        } else {
            numPacketDropped++;
            printf("%011.3lfms: p%d arrives, dropped\n", 
                timeSubtract(&startTime, &currentTime),
                numPacketProduced);
        }

        pthread_mutex_unlock(&m);
    }
    pthread_cleanup_pop(1);
    return NULL;
}

void *server(void *arg) {

    int flag = 1;
    int transimit = 1;
    Packet *tmp;
    int serverId = *(int *)arg;
    while (flag) {
        transimit = 0;
        pthread_mutex_lock(&m);
        if (ListEmpty(Q2) && signalDetect != 1 && numPacketServed + numPacketDropped != n) {
            pthread_cond_wait(&cv, &m);
        }
        if (!ListEmpty(Q2)) {
            ListElem *elem = listNext(Q2);
            tmp = (elem->obj);

            gettimeofday(&currentTime, NULL);

            printf("%011.3lfms: p%d leaves Q2, time in Q2 = %.3lfms\n", 
                timeSubtract(&startTime, &currentTime),
                tmp->packetId, 
                timeSubtract(&(tmp->arriveQ2), &currentTime));            
            
            timeSpentInQ2 += timeSubtract(&(tmp->arriveQ2), &currentTime);

            gettimeofday(&currentTime, NULL);
            tmp->arriveServer = currentTime;
            printf("%011.3lfms: p%d begins service at S%d, requesting %dms of service\n", 
                timeSubtract(&startTime, &currentTime),
                tmp->packetId, serverId, tmp->serviceTime);
            
            packetServiceTime += tmp->serviceTime;

            transimit = 1;

            free(elem);
        }
        pthread_mutex_unlock(&m);
        if (transimit) {
            usleep(tmp->serviceTime * mul);

            pthread_mutex_lock(&m);
            gettimeofday(&currentTime, NULL);
            double timeSpentTotal = timeSubtract(&(tmp->arrives), &currentTime);
            double timeSpentInServer = timeSubtract(&(tmp->arriveServer), &currentTime);
            printf("%011.3lfms: p%d departs from S%d, service time = %.3lfms, time in the system = %.3lfms\n", 
                timeSubtract(&startTime, &currentTime),
                tmp->packetId, 
                serverId, 
                timeSpentInServer,
                timeSpentTotal);
            pthread_mutex_unlock(&m);

            timeSpentInServers[serverId-1] += timeSpentInServer;

            averageTimeSpentInSystem = (averageTimeSpentInSystem * numPacketServed + timeSpentTotal / mul) / (numPacketServed + 1);
            averageTimeSquareSpentInSystem = (averageTimeSquareSpentInSystem * numPacketServed + 
                                                timeSpentTotal * timeSpentTotal / mul / mul) / (numPacketServed + 1);
            numPacketServed++;
        }

        pthread_mutex_lock(&m);
        if (numPacketServed + numPacketDropped == n || signalDetect == 1) {
            flag = 0;
        }
        pthread_mutex_unlock(&m);
        pthread_cond_broadcast(&cv);
    }
    return NULL;

}

void *signalCatch() {
    int sig;
    sigwait(&set, &sig);

    pthread_cancel(tockenArriveThreadId);
    pthread_cancel(packetArriveThreadId);

    pthread_mutex_lock(&m);

    signalDetect = 1;
    gettimeofday(&currentTime, NULL);
    printf("\n%011.3lfms: SIGINT caught, no new packets or tokens will be allowed\n", timeSubtract(&startTime, &currentTime));
    while (!ListEmpty(Q1)) {
        ListElem *elem = listNext(Q1);
        gettimeofday(&currentTime, NULL);
        printf("%011.3lfms: p%d removed from Q1\n", timeSubtract(&startTime, &currentTime), elem->obj->packetId);
        free(elem);
    }
    while (!ListEmpty(Q2)) {
        ListElem *elem = listNext(Q2);
        gettimeofday(&currentTime, NULL);
        printf("%011.3lfms: p%d removed from Q2\n", timeSubtract(&startTime, &currentTime), elem->obj->packetId);
        free(elem);
    }
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&m);
    return NULL;
}

int main(int argc, char *argv[])  
{
    parseCommandLine(argc, argv);
    initData();
    printParameters();
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, 0);

    gettimeofday(&startTime, NULL);
    printf("%011.3fms, emulation begins\n", timeSubtract(&startTime, &startTime));
    
    pthread_create(&tockenArriveThreadId, NULL, tockenArrive, NULL);
    pthread_create(&packetArriveThreadId, NULL, packetArrive, NULL);
    pthread_create(&serverThreadId[0], NULL, server, (void *)(&serverIds[0]));
    pthread_create(&serverThreadId[1], NULL, server, (void *)(&serverIds[1]));
    pthread_create(&signalThreadId, NULL, signalCatch, NULL);

    pthread_join(tockenArriveThreadId, NULL);
    pthread_join(packetArriveThreadId, NULL);
    pthread_join(serverThreadId[0], NULL);
    pthread_join(serverThreadId[1], NULL);

    gettimeofday(&currentTime, NULL);
    totalEmulationTime = timeSubtract(&startTime, &currentTime);
    printf("%011.3fms: emulation ends\n", totalEmulationTime);

    freeData();
    printStatistics();

}