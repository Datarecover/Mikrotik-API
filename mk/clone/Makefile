GCC_FLAGS =  -Wall -Wno-unused-result
CC        = gcc
CFLAGS    = -g -O2
LIBS      =


mkclone: mkclone.o ../md5.o ../api.o
	$(CC) $(LIBS) -o mkclone md5.o api.o mkclone.o

.c.o:
	$(CC) -c $(CFLAGS) $(GCC_FLAGS) $< 

.PHONY: clean

clean:
	@rm -f mkclone mkclone.o md5.o api.o

