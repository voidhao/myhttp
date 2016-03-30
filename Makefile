serv: myhttp.o
	gcc -o serv myhttp.o
myhttp.o: myhttp.c
	gcc -c myhttp.c
clean:
	rm -rf *.o serv
