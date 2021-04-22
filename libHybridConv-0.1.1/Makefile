CC=gcc

INCDIR=
LIBDIR=

#CFLAGS=-g -O0 -msse -msse2 -fomit-frame-pointer -funroll-loops -ffast-math -Wall -Wextra 
CFLAGS=-O2 -msse -msse2 -fomit-frame-pointer -funroll-loops -ffast-math -Wall -Wextra 
LFLAGS=-lm -lfftw3f


OBJ=libHybridConv.o hcTest.o


all : hcTest

hcTest : $(OBJ)
	$(CC) -o hcTest $(OBJ) $(LIBDIR) $(LFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c $< $(INCDIR)

install:
	echo Nothing to be done

clean:
	rm $(OBJ) hcTest

