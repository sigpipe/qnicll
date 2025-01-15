#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void util_print_all(char *str) {
  unsigned char c;
  while(c=*str++) {
    if (((c>=' ')&&(c<127))||(c=='\n')) printf("%c", c);
    else printf("<%d>", c);
  }
}

char *util_readfile(char *fname) {
  int fd;
  ssize_t sz;
  static char buf[256];
  fd = open(fname, O_RDONLY, 0);
  if (fd<0) {
    printf("ERR: util_readfile: cant open %s\n", fname);
    return 0;
  }
  sz = read(fd, buf, 256);
  buf[sz]=0;
  return buf;  
}

char *util_read_ipaddr_dot_txt(void) {
  char *ipaddr;
  int i;
  ipaddr = util_readfile("ipaddr.txt");
  for (i=0;i<strlen(ipaddr);++i)
    if (ipaddr[i]<=' ') {
      ipaddr[i]=0;
      break;
    }
  return ipaddr;
}
