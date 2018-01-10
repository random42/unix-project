#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include "header.h"

pid_t pid;
unsigned long genoma;
char* nome;
int msgid;

void init() {
  pid = getpid();
  msgid = msgget(MSG_KEY,0);
  received = malloc(sizeof(message)*4096);
  received_length = 0;
  int i = 0;
  message m;
  
  while (i < 1) {
    msgsnd(msgid,&m,1,pid,0);
    printf("Sent genoma %lu, pid %d",m.genoma,m.pid);
    i++;
  }
}

quit() {

}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  init();
}
