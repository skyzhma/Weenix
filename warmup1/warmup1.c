#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "my402list.h"

typedef struct tagData {
    char operation;
    unsigned int timeStamp;
    char desc[30];
    int number;
    int amount;
    char numberStr[15];
    char amountStr[15];
} Data;

int sort(My402List *list, Data *data) {

    if (My402ListEmpty(list)) {
        My402ListAppend(list, (void *)data);
        return TRUE;
    }

    My402ListElem *elem;
    My402ListElem *lastElem = My402ListFirst(list);
    int insert = 0;
    for (elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
        Data *listData = (Data *)(elem->obj);
        if (listData->timeStamp == data->timeStamp) {
            return FALSE;
        } else if (listData->timeStamp > data->timeStamp) {
            My402ListInsertBefore(list, (void *)data, elem);
            insert = 1;
            break;
        }
        lastElem = elem;
    }
    if (!insert) {
        My402ListInsertAfter(list, (void *)data, lastElem);
    }
    return TRUE;
}

void addBalance(My402List *list) {

    if (My402ListEmpty(list)) {
        return;
    }
    My402ListElem *elem;
    int number = 0;
    for (elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
        Data *data = (Data *)(elem->obj);
        // printf("%d %d\n", data->amount, data->number);

        if (data->operation == '+') {
            number += data->amount;
        } else {
            number -= data->amount;
        }
        data->number = number;

    }
}

void balanceToStr(char *dest, int number) {

    if (number < 0) {
        number = -number;
    }

    int dollars = number / 100;
    int cents = number % 100;

    if (dollars >= 10000000) {
        strcpy(dest, "?,???,???.??");
        return ;
    }

    char x[30];
    sprintf(x, "%d", dollars);
    int length = strlen(x);
    int numDot = (length % 3 == 0 ? length / 3 - 1 : length / 3);
    int count = length + numDot - 1;

    for (int i = length - 1, j = 0; i >= 0; i--, j++) {
        if (j && j % 3 == 0) {
            dest[count] = ',';
            count--;
        }
        dest[count] = x[i];
        count--;
    }

    count = length + numDot;
    dest[count] = '.';
    count+=2;
    for (int i = 0; i < 2; i++) {
        int y = cents % 10;
        dest[count--] = '0' + y;
        cents /= 10;
    }    
    dest[count+3] = '\0';

}

void parseTime(unsigned int timeStamp, char *buf) {

    time_t t = timeStamp;
    strcpy(buf, ctime(&t));
    int count = 11;
    for (int i = 20; i < 24; i++, count++) {
        buf[count] = buf[i];
    }
    buf[count] = '\0';

}

void printList(My402List *list) {
    
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");

    My402ListElem *elem=NULL;
    char buf[25];
    for (elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
        Data *data = (Data *)(elem->obj);
        parseTime(data->timeStamp, buf);

        balanceToStr(data->amountStr, data->amount);
        balanceToStr(data->numberStr, data->number);


        // printf("%d %d\n", data->amount, data->number);


        if (data->operation == '+') {
            printf("| %-15s | %-24s | %13s  | ", buf, data->desc, data->amountStr);
        } else {
            printf("| %-15s | %-24s | (%12s) | ", buf, data->desc, data->amountStr);
        }
        if (data->number >= 0) {
            printf("%13s  |\n", data->numberStr);
        } else {
            printf("(%12s) |\n", data->numberStr);
        }

    }
    printf("+-----------------+--------------------------+----------------+----------------+\n");

}

void readOperation(char *buf, Data *data, int lineNumer) {

    strcpy(&data->operation, buf);
    if (data->operation != '-' && data->operation != '+') {
        fprintf(stderr, "Error: In line %d, invalid operation symbol\n", lineNumer);
        exit(-1);
    }
}

void readTimeStamp(char *buf, Data *data, int lineNumber) {
    if (strlen(buf) >= 11 || strlen(buf) == 0) {
        fprintf(stderr, "Error: In line %d, Invalid TimeStamp\n", lineNumber);
        exit(-1);
    }

    time_t current;
    time(&current);
    unsigned int record = 0;
    for (int i = 0; i < strlen(buf); i++) {
        record *= 10;
        record += buf[i] - '0';
    }
    if (record > current) {
        fprintf(stderr, "Error: In line %d, Invalid TimeStamp that's greater than current timeStamp\n", lineNumber);
        exit(-1);
    }
    data->timeStamp = record;

}

void readAmount(char *buf, Data *data, int lineNumber) {
    
    int i = 0;
    int amount = 0;

    if (strlen(buf) == 0 || buf[0] == '-') {
        fprintf(stderr, "Error: In line %d, the transaction amount is not a number or is negative\n", lineNumber);
        exit(-1);
    }
    for (; i < strlen(buf); i++) {
        if (buf[i] != '.') {
            amount *= 10;
            amount += buf[i] - '0';
        } else {
            break;
        }
    }
    if (i >= 7) {
        fprintf(stderr, "Error: In line %d, the number to the left of the decimal point exceeds 7\n", lineNumber);
        exit(-1);
    }

    amount = amount * 10 + (buf[i+1] - '0');
    amount = amount * 10 + (buf[i+2] - '0');

    if (i+3 < strlen(buf)) {
        fprintf(stderr, "Error: In line %d, the number to the right of the decimal point exceeds 2\n", lineNumber);
        exit(-1);
    }

    
    data->amount = amount;
}

void readDesc(char *string, Data *data, int i) {

    int j = strlen(string) - 1;
    while(*(string+i) == ' ') {
        i++;
    }
    while(*(string+j) == ' ' || *(string+j) == 10 || *(string+j) == 13) {
        j--;
    }

    int count = 0;
    for (; i <= j; i++) {
        data->desc[count++] = string[i];
    }
    data->desc[24] = '\0';

}

void readData(FILE *fp, My402List *list) {
    char tmp[1025];
    char buf[1024];
    int lineNumber = 1;
    while (fgets(tmp, sizeof(tmp), fp)!=NULL) {
        if (strlen(tmp) > 1024) {
            fprintf(stderr, "Error, reach 1024 characters limit\n");
            exit(-1);
        }

        Data *data = (Data *)malloc(sizeof(Data));
        memset(data, 0, sizeof(Data));
        int length = strlen(tmp);
        int count = 0;
        int lastTab = -1;
        for (int i = 0; i < length; i++) {
            if (tmp[i] == '\t') {
                count++;
                strncpy(buf, tmp+lastTab+1, i-1-lastTab);
                buf[i-1-lastTab] = '\0';
                if (count == 1) {
                    readOperation(buf, data, lineNumber);
                } else if (count == 2) {
                    readTimeStamp(buf, data, lineNumber);
                } else if (count == 3) {
                    readAmount(buf, data, lineNumber);

                }
                
                lastTab = i;
            }
        }
        if (count != 3) {
            fprintf(stderr, "Error: in line %d, number of tab characters is not equal to 3\n", lineNumber);
            exit(-1);
        }

        readDesc(tmp, data, lastTab + 1);

        if (!sort(list, data)) {
            fprintf(stderr, "Error: in line %d, Identical timestamp\n", lineNumber);
            exit(-1);;
        }

        lineNumber++;
    }

    addBalance(list);

}

void parseCommand(int argc, char *argv[], My402List *list) {
    argc--;argv++;
    char fileName[1024];
    FILE *fp = stdin;
    if (argc == 0) {
        fprintf(stderr, "usage: warmup1 sort [tfile]\n");
        exit(-1);
    }
    if (strcmp(*argv, "sort") == 0) {

        if (argc - 1 > 0) {

            sscanf(*(argv+1), "%s", fileName);

            if (access(fileName, 0) != -1) {
                struct stat path; 
                stat(fileName, &path);
                if(S_ISREG(path.st_mode) == 0) {
                    fprintf(stderr, "input file \"%s\" is a directory\n", fileName);
                    exit(-1);
                }
                if (access(fileName, 2) == -1) {
                    fprintf(stderr, "input file \"%s\" cannot be opened - access denies\n", fileName);
                    exit(-1);
                }
            } else {
                fprintf(stderr, "input file \"%s\" does not exist\n", fileName);
                exit(-1);
            }

            fp = fopen(fileName, "r");

        }
        
        readData(fp, list);
        
    } else {
        fprintf(stderr, "malformed command,\"%s\" is not a valid commandline option\n", *argv);
        fprintf(stderr, "usage: warmup1 sort [tfile]\n");
        exit(-1);
    }

}

int main(int argc, char *argv[]) {

    My402List *list = (My402List *)malloc(sizeof(My402List));
    My402ListInit(list);
    parseCommand(argc, argv, list);
    printList(list);

}