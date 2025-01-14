OBJS=qregc.o qnicll.o qna.o qna_usb.o

libqnicll.a: $(OBJS)
	ar r $@ $(OBJS)

clean:
	rm -f tst n *.o *.a

all: tst n libqnicll.a

INCS  = qnicll.h qregc.h qna.h qna_usb.h util.h qnicll_internal.h 
SRCS  = qnicll.c qregc.c qna.c qna_usb.c util.c 

tst: tst.c $(SRCS) $(INCS)
	gcc tst.c $(SRCS) -liio -lm -o tst

n: n.c $(SRCS) $(INCS)
	gcc n.c $(SRCS) -liio -lm -o n
