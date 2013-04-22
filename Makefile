OBJ=test.o lz.o palm.o
HDR=lz.h palm.h

test:$(OBJ)
	gcc -o test $(OBJ) -g
test.o:test.c lz.h palm.h
	gcc -c test.c -g
lz.o:lz.c lz.h
	gcc -c lz.c -g
palm.o:palm.c palm.h
	gcc -c palm.c -g

clean:
	rm *.o test
