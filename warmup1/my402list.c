#include "my402list.h"
#include "cs402.h"
#include "stdlib.h"
#include "string.h"
#include <stdio.h>
#include <sys/time.h>

int My402ListLength(My402List* list) {
    return list->num_members;
}

int My402ListEmpty(My402List* list) {
    return list->num_members == 0 ? TRUE : FALSE;
}

int My402ListAppend(My402List* list, void* data) {
    My402ListElem *elem = (My402ListElem*)malloc(sizeof(My402ListElem));
    memset(elem, 0, sizeof(My402ListElem));
    elem->obj = data;

    elem->next = &(list->anchor);
    list->anchor.prev->next = elem;
    elem->prev = list->anchor.prev;
    list->anchor.prev = elem;

    list->num_members++;
    
    return TRUE;
}

int My402ListPrepend(My402List* list, void* data) {
    My402ListElem *elem = (My402ListElem*)malloc(sizeof(My402ListElem));
    elem->obj = data;

    elem->next = list->anchor.next;
    elem->prev = &(list->anchor);
    list->anchor.next->prev = elem;
    list->anchor.next = elem;

    list->num_members++;
    return TRUE;
}

void My402ListUnlink(My402List* list, My402ListElem* elem) {

    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
    list->num_members--;
    free(elem);
}

void My402ListUnlinkAll(My402List* list) {

    My402ListElem *head = &(list->anchor);
    My402ListElem *p = head->next;
    while (p != head) {
        My402ListElem *next = p->next;
        free(p);
        p = next;
    }
    head->next = &list->anchor;
    head->prev = &list->anchor;
    list->num_members = 0;
}

int My402ListInsertAfter(My402List* list, void* data, My402ListElem* elem) {
    if (elem == NULL) {
        return My402ListAppend(list, data);
    }
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    newElem->obj = data;
    newElem->next = elem->next;
    elem->next->prev = newElem;
    newElem->prev = elem;
    elem->next = newElem;
    list->num_members++;
    return TRUE;
}

int My402ListInsertBefore(My402List* list, void* data, My402ListElem* elem) {
    if (elem == NULL) {
        return My402ListPrepend(list, data);
    }
    My402ListElem *newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
    newElem->obj = data;
    elem->prev->next = newElem;
    newElem->prev = elem->prev;
    newElem->next = elem;
    elem->prev = newElem;
    list->num_members++;
    return TRUE;
}

My402ListElem *My402ListFirst(My402List* list) {
    return list == NULL ? NULL : list->anchor.next;
}

My402ListElem *My402ListLast(My402List* list) {
    return list == NULL ? NULL : list->anchor.prev;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* elem) {
    if (elem->next == &(list->anchor)) {
        return NULL;
    }
    return elem->next;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem) {
    if (elem->prev == &(list->anchor)) {
        return NULL;
    }
    return elem->prev;
}

My402ListElem *My402ListFind(My402List* list, void* data) {
    My402ListElem *head = &(list->anchor);
    My402ListElem *p = list->anchor.next;
    while (p != head && p->obj != data) {
        p = p->next;
    }
    if (p == head) {
        return NULL;
    }
    return p;
}

int My402ListInit(My402List* list) {

    memset(list, 0, sizeof(My402List));
    list->anchor.next = &list->anchor;
    list->anchor.prev = &list->anchor;

    return TRUE;
}

