#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include "header.h"

pid_t pid;
unsigned long genoma;
char* nome;
int msqid;
int msgsize;
message* received;
int received_length;

unsigned long mcd(unsigned long a, unsigned long b) {
  unsigned long r;
  while (a % b != 0) {
    r = a%b;
    a = b;
    b = r;
  }
  return b;
}

void ascolta() {}

void init() {
  pid = getpid();
  msqid = msgget(MSG_KEY,0);
  msgsize = sizeof(message)-sizeof(long);
}

void quit() {

}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  init();
  signal(SIGTERM,quit);
}
