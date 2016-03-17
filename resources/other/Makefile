CC=gcc -g -D__MACOSX_CORE__
LIBS=-lsndfile -lportaudio
EXE=convolution
FLAGS=-c -Wall
SRCS=convolution.c

all: $(EXE)

$(EXE): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf $(EXE).dSYM