#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "header.h"
#include "child.h"
#include "sem.h"

extern int msq_match;
extern int msq_contact;
extern int msgsize;

extern short sem_num;

extern int sem_start;
extern int sem_match;

extern char* stack[];
extern int stack_length;
extern int debug_info;

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
  add_func("trova_divisori");
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
  rm_func();
}

void do_nothing(int sig){}

void set_signals(void(quit)(int),void(debug)(int)) {
  add_func("set_signals");
  struct sigaction sig_term;
  struct sigaction sig_debug;
  struct sigaction sig_do_nothing;
  struct sigaction sig_ignore;
  // handlers
  sig_ignore.sa_handler = SIG_IGN;
  sig_debug.sa_handler = debug;
  sig_do_nothing.sa_handler = do_nothing;
  sig_term.sa_handler = quit;
  // Handlers non interrompibili
  sigfillset(&sig_term.sa_mask);
  sigfillset(&sig_debug.sa_mask);
  // Handler interrompibile
  sigemptyset(&sig_do_nothing.sa_mask);
  // Setta gli handler dei segnali
  sigaction(SIGTERM,&sig_term,NULL);
  sigaction(SIGALRM,&sig_do_nothing,NULL);
  sigaction(SIGUSR2,&sig_debug,NULL);
  sigaction(SIGSEGV,&sig_debug,NULL);
  sigaction(SIGABRT,&sig_debug,NULL);
  sigaction(SIGILL,&sig_debug,NULL);
  sigaction(SIGINT,&sig_ignore,NULL);
  rm_func();
}

void fine_match() {
  add_func("fine_match");
  // segnalano di aver finito il match
  add_match(sem_num,-1);
  // riprendono l'esecuzione se il gestore aumenta il semaforo
  add_start(sem_num,-1);
  rm_func();
}

void msq_init() {
  add_func("msq_init");
  msgsize = sizeof(message)-sizeof(unsigned long);
  if ((msq_match = msgget(MSG_MATCH,0)) == -1) {
    raise(SIGTERM);
  }
  if ((msq_contact = msgget(MSG_CONTACT,0)) == -1) {
    raise(SIGTERM);
  }
  rm_func();
}

void ready() {
  add_func("ready");
  add_start(sem_num,-1);
  add_start(sem_num,-1);
  rm_func();
}
