all: tst n c

INCS  = qnicll.h qregc.h qna.h qna_usb.h util.h qnicll_internal.h 
SRCS  = qnicll.c qregc.c qna.c qna_usb.c util.c 
DEPS  = Makefile $(SRCS) $(INCS)

tst: tst.c $(DEPS)
	gcc tst.c $(SRCS) -liio -lm -o tst

n: n.c $(DEPS)
	gcc n.c $(SRCS) -liio -lm -o n

# qnic commands
c: c.c $(DEPS)
	gcc c.c $(SRCS) -liio -lm -o c
