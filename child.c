#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "header.h"
#include "child.h"

extern int msq_match;
extern int msq_start;
extern int msq_contact;
extern int msgsize;

extern unsigned long genoma;
extern unsigned long* divisori;
extern int div_length;

unsigned long mcd(unsigned long a, unsigned long b) {
  unsigned long r;
  while (a % b != 0) {
    r = a%b;
    a = b;
    b = r;
  }
  return b;
}

void trova_divisori() {
  // inizializza l'array di divisori
  divisori = malloc(sizeof(unsigned long) * (genoma > 60 ? genoma/5 : genoma));
  divisori[0] = genoma;
  unsigned long i = genoma/2;
  int index = 1;
  while (i > 0) {
    if (genoma % i == 0) {
      divisori[index++] = i;
    }
    i--;
  }
  div_length = index;
}

void do_nothing(int sig){}

void set_signals(void(quit)(int),void(debug)(int)) {
  struct sigaction sig_term;
  struct sigaction sig_debug;
  struct sigaction sig_do_nothing;
  // handlers
  sig_debug.sa_handler = debug;
  sig_do_nothing.sa_handler = do_nothing;
  sig_term.sa_handler = quit;
  // masks
  sigfillset(&sig_term.sa_mask);
  sigfillset(&sig_debug.sa_mask);
  sigfillset(&sig_do_nothing.sa_mask);
  // Setta gli handler dei segnali
  sigaction(SIGTERM,&sig_term,NULL);
  sigaction(SIGALRM,&sig_debug,NULL);
  sigaction(SIGUSR1,&sig_do_nothing,NULL);
  sigaction(SIGUSR2,&sig_debug,NULL);
}

void msq_init() {
  msgsize = sizeof(message)-sizeof(unsigned long);
  if ((msq_match = msgget(MSG_MATCH,0)) == -1) {
    raise(SIGTERM);
  }
  if ((msq_start = msgget(MSG_START,0)) == -1) {
    raise(SIGTERM);
  }
  if ((msq_contact = msgget(MSG_CONTACT,0)) == -1) {
    raise(SIGTERM);
  }
}

void ready() {
  message m;
  m.mtype = getpid();
  while (msgsnd(msq_start,&m,msgsize,0) == -1 && errno == EINTR) continue;
}
