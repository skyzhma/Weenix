warmup2: warmup2.o mylist.o utils.o
	gcc -g warmup2.o mylist.o utils.o -o warmup2 -lpthread -lm

warmup2.o: warmup2.c mylist.h utils.h
	gcc -g -c -Wall warmup2.c

mylist.o: mylist.c mylist.h
	gcc -g -c -Wall mylist.c

utils.o: utils.c utils.h
	gcc -g -c -Wall utils.c

clean:
	rm -f *.o warmup2



