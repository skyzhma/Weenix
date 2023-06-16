#include<sys/time.h>

typedef struct tagPacket {
    int serviceTime;
    int numTokenNeeded;
    int interArrivalTime;
    int packetId;
    struct timeval arrives;
    struct timeval arriveQ1;
    struct timeval arriveQ2;
    struct timeval arriveServer;

} Packet;

typedef struct tagListElem {
    Packet *obj;
    struct tagListElem *next;
    struct tagListElem *prev;
} ListElem;

typedef struct tagList {
    int num_members;
    ListElem anchor;
} List;

extern int ListLength(List*);
extern int ListEmpty(List*);
extern int ListAppend(List*, ListElem*); 
extern ListElem *listNext(List*);

extern int listNextPacketTokenNeed(List*);

extern int ListInit(List*);

extern Packet *packetInit(int, int, int, int);
extern ListElem *elemInit(Packet *);
