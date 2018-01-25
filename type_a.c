#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "header.h"
#include "type_a.h"

#define MAX_CONTACT 2048



// Proprie caratteristiche
unsigned long genoma;
char* nome;

// mcd target
unsigned long target;

// Numero di contatti rifiutati nel corrente target
unsigned long contacts;

// Divisori del proprio genoma
unsigned long* divisors;
int div_length;

int msq_match;
int msq_start;
int msgsize;


void find_divisors() {
  divisors[0] = genoma;
  unsigned long i = genoma/2;
  int index = 1;
  while (i > 0) {
    if (genoma % i == 0) {
      divisors[index++] = i;
    }
    i--;
  }
  div_length = index;
}


unsigned long mcd(unsigned long a, unsigned long b) {
  unsigned long r;
  while (a % b != 0) {
    r = a%b;
    a = b;
    b = r;
  }
  return b;
}


void abbassa_target() {
  int i = 0;
  while (i < div_length && target != divisors[i]) i++;
  if (target > 1) {
    target = divisors[i+1];
  }
}


// registra l'id in first_contact o sec_contact
void push_contact(unsigned int id) {
  contacts++;
  if (contacts > INIT_PEOPLE - 1) {
    abbassa_target();
    contacts = 0;
  }
}


// accetta il processo B e si accoppia
void accetta(pid_t pid) {
  message s;
  s.data = 1;
  s.mtype = pid;
  s.pid = getpid();
  s.partner = pid;
  // Messaggio di assenso al processo B
  if (msgsnd(msq_match,&s,msgsize,0) == -1 && errno != EINTR) {
    print_error();
    raise(SIGTERM);
  }
  s.mtype = getppid();
  // Messaggio per il gestore
  if (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) {
    print_error();
    raise(SIGTERM);
  }
  // Aspetta il segnale di terminazione o quello di continue
  // se il processo B e' terminato prima tramite birth_death
  pause();
}


// rifiuta il processo B
void rifiuta(pid_t pid) {
  message s;
  s.data = 0;
  s.mtype = pid;
  s.pid = getpid();
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
}

void ascolta() {
  message r;
  message s;
  while (1) {
    // ricevo il messaggio
    while (msgrcv(msq_match,&r,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
    if (mcd(genoma,r.genoma) >= target) { // l'mcd corrisponde al target
      accetta(r.pid);
    } else {
      rifiuta(r.pid);
      push_contact(r.id);
    }
  }
}

void do_nothing(int sig){}

void set_signals() {
  struct sigaction sig_term;
  sig_term.sa_handler = quit;
  sigfillset(&sig_term.sa_mask);
  // Setta gli handler dei segnali
  sigaction(SIGTERM,&sig_term,NULL);
  signal(SIGUSR1,do_nothing);
  signal(SIGALRM,sterile);
}


void init() {
  set_signals();
  // inizializza l'array di divisori
  divisors = malloc(sizeof(unsigned long)*genoma);
  // le variabili
  contacts = 0;
  msgsize = sizeof(message)-sizeof(long);
  // coda di messaggi
  if ((msq_match = msgget(MSG_MATCH,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  if ((msq_start = msgget(MSG_START,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  // cerca i divisori
  find_divisors();
  // target iniziale = genoma
  target = divisors[0];
}

void quit(int sig) {
  exit(EXIT_SUCCESS);
}

void start() {
  alarm(10);
  ascolta();
}

void ready() {
  message m;
  m.mtype = getpid();
  if (msgsnd(msq_start,&m,msgsize,0) == -1 && errno != EINTR) {
    print_error();
    raise(SIGTERM);
  }
}

void sterile(int sig) {
  printf("A sterile: pid %d\tgenoma %lu\ttarget %lu\n",getpid(),genoma,target);
}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  init();
  // Segnala al gestore di essere pronto
  ready();
  // Attende il segnale di start
  pause();
  start();
}
