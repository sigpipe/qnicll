OBJS=qregc.o qnicll.o qna.o qna_usb.o

all: tst libqnicll.a

libqnicll.a: $(OBJS)
	ar r $@ $(OBJS)

clean:
	rm -f tst *.o *.a

INCS  = qnicll.h qregc.h qna.h qna_usb.h util.h qnicll_internal.h 
SRCS  = qnicll.c qregc.c qna.c qna_usb.c util.c 
SRCS += tst.c

tst: $(SRCS) $(INCS)
	gcc $(SRCS) -liio -lm -o tst
