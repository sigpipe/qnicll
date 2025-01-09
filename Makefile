OBJS=qregc.o qnicll.o qna.o qna_usb.o

all: tst libqnicll.a

libqnicll.a: $(OBJS)
	ar r $@ $(OBJS)

tst: tst.c qregc.c qnicll.h qnicll_internal.h qnicll.c qna.h qna.c
	gcc tst.c qregc.c qna.c qna_usb.c qnicll.c -liio -lm -o tst

clean:
	rm -f tst *.o *.a
