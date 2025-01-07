all: tst



tst: tst.c qregc.c qnicll.h qnicll_internal.h qnicll.c qnac.h qnac.c
	gcc tst.c qregc.c qnac.c qnicll.c -liio -lm -o tst
