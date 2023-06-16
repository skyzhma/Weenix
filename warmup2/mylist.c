#include "mylist.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif /* ~TRUE */

int ListLength(List* list) 
{
    return list->num_members;
}

int ListEmpty(List* list) {
    return list->num_members == 0 ? TRUE : FALSE;
}

int ListAppend(List* list, ListElem* elem) 
{
    if (list->anchor.next == NULL) {
        list->anchor.prev = elem;
        elem->prev = &(list->anchor);
        list->anchor.next = elem;
        elem->next = &(list->anchor);
    } else {
        elem->next = &(list->anchor);
        list->anchor.prev->next = elem;
        elem->prev = list->anchor.prev;
        list->anchor.prev = elem;
    }
    list->num_members++;
    
    return TRUE;
}

ListElem *listNext(List* list) {
    ListElem *res;
    res = list->anchor.next;
    if (list->num_members == 1) {
        list->anchor.next = NULL;
        list->anchor.prev = NULL;
        
    } else {
        list->anchor.next = res->next;
        res->next->prev = &list->anchor;
    } 
    list->num_members--;
    return res;
}

int listNextPacketTokenNeed(List* list) {
    return list->anchor.next->obj->numTokenNeeded;
}

int ListInit(List* list) 
{
    list->num_members = 0;

    ListElem *anchor = (ListElem*)malloc(sizeof(ListElem));
    anchor->obj = NULL;
    anchor->next = NULL;
    anchor->prev = NULL;
    list->anchor = (*anchor);

    return TRUE;
}

Packet *packetInit(int serverceTime, int numTokenNeeded, int interArrivalTime, int packetId) {
    Packet *newPacket = (Packet *)malloc(sizeof(Packet));
    newPacket->serviceTime = serverceTime;
    newPacket->numTokenNeeded = numTokenNeeded;
    newPacket->interArrivalTime = interArrivalTime;
    newPacket->packetId = packetId;
    return newPacket;
}

ListElem *elemInit(Packet *data) {
    ListElem *elem = (ListElem*)malloc(sizeof(ListElem));
    memset(elem, 0, sizeof(ListElem));
    elem->obj = data;
    return elem;
}
