#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include "header.h"

SIGTERM
pid_t pid;
unsigned long genoma;
char* nome;
int msgid;
message* received;
int received_length;

void init() {
  pid = getpid();
  msgid = msgget(MSG_KEY,0);
  received = malloc(sizeof(message)*4096);
  received_length = 0;
  int i = 0;
  while (i < 10) {
    msgrcv(msgid,received[received_length++],1,pid,0);
    printf("Received genoma %lu, pid %d",received[i].genoma,received[i].pid);
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
