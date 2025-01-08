OBJS=qregc.o qnicll.o qnac.o

all: tst libqnicll.a

libqnicll.a: $(OBJS)
	ar r $@ $(OBJS)

tst: tst.c qregc.c qnicll.h qnicll_internal.h qnicll.c qnac.h qnac.c
	gcc tst.c qregc.c qnac.c qnicll.c -liio -lm -o tst

clean:
	rm -f tst
