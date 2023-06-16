#include <stdio.h>
#include <math.h>

int main (int argc, char *argv[])
{
    printf("%d\n", *(argv+1) == NULL);
}