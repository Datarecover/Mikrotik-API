GCC_FLAGS =  -Wall -Wno-unused-result
CC        = gcc
CFLAGS    = -g -O2
LIBS      = 


mktest: mktest.o ../md5.o ../api.o
	$(CC) $(LIBS) -o mktest md5.o api.o mktest.o

.c.o:
	$(CC) -c $(CFLAGS) $(GCC_FLAGS) $< 

.PHONY: clean

clean:
	@rm -f mktest mktest.o md5.o api.o

