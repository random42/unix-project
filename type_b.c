#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include <errno.h>
#include "header.h"
#include "shm.h"
#include "type_b.h"


// Proprie caratteristiche
unsigned long genoma;
char* nome;
unsigned int id;

// Array degli ID dei processi A gia' contattati
unsigned int black_list[SHM_LENGTH];
int black_list_length;

// Divisori del proprio genoma
unsigned long* divisors;
int div_length;

//mcd target
unsigned long target;

int msq_match;
int msq_start;
int msgsize;

int shmid;
void* shmptr;



/* Aquisisce la memoria condivisa con il gestore */
void shm_init() {
  int flag = 0600;
  int size = sizeof(int) + (sizeof(a_person) * SHM_LENGTH);
  if ((shmid = shmget(SHM_KEY,size,flag)) == -1) {
    printf("Failed to get shared memory segment.\n");
    raise(SIGTERM);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    printf("Failed to attach shared memory segment.\n");
    raise(SIGTERM);
  }
}

/* Esegue il detach della memoria condivisa */
void shm_detach() {
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    raise(SIGTERM);
  }
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

int not_black_list(unsigned int id) {
  for (int i = 0; i < black_list_length;i++) {
    if (id == black_list[i]) {
      return 0;
    }
  }
  return 1;
}

/* Manda un messaggio con il pid del processo A accoppiato
al gestore, ma con mtype uguale al proprio pid*/
void accoppia(pid_t pid) {
  message s;
  s.mtype = getpid();
  s.data = 0;
  s.pid = getpid();
  s.partner = pid;
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
  // Attende il segnale di terminazione
  pause();
}

char contatta(pid_t pid) {
  message s;
  message r;
  s.mtype = pid;
  s.pid = getpid();
  s.genoma = genoma;
  s.id = id;
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
  while (msgrcv(msq_match,&r,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
  return r.data;
}

void cerca_target() {
  int* length = shmptr;
  a_person* a = shmptr+sizeof(int);
  for (int i = 0; i < *length;i++) {
    if (a[i].valid && mcd(genoma,a[i].genoma) >= target && not_black_list(a[i].id)) {
      if (!contatta(a[i].pid)) { // se viene rifiutato aggiunge il processo A nella black_list
        black_list[black_list_length++] = a[i].id;
      } else { // altrimenti si accoppia
        accoppia(a[i].pid);
      }
    }
  }
}

void cerca() {
  int i = 0;
  while (1) {
    if (i == div_length) { // resetta target = genoma e cancella la black_list
      i = 0;
      black_list_length = 0;
    }
    target = divisors[i++];
    cerca_target();
  }
}

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

void set_signals() {
  struct sigaction sig_term;
  sig_term.sa_handler = quit;
  sigfillset(&sig_term.sa_mask);
  // Setta gli handler dei segnali
  sigaction(SIGTERM,&sig_term,NULL);
  signal(SIGUSR1,do_nothing);
  signal(SIGALRM,sterile);
}

void do_nothing(int sig){}

void init() {
  set_signals();
  // inizializza le variabili
  divisors = malloc(sizeof(unsigned long) * (genoma > 60 ? genoma/5 : genoma));
  black_list_length = 0;
  msgsize = sizeof(message)-sizeof(long);
  // la coda di messaggi
  if ((msq_match = msgget(MSG_MATCH,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  if ((msq_start = msgget(MSG_START,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  // la memoria condivisa
  shm_init();
  // trova i divisori
  find_divisors();
}

void quit(int sig) {
  shm_detach();
  exit(EXIT_SUCCESS);
}

void start() {
  alarm(10);
  cerca();
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
  printf("B sterile: pid %d\tgenoma %lu\ttarget %lu\n",getpid(),genoma,target);
}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  id = strtoul(argv[3],NULL,10);
  init();
  // Segnala al gestore di essere pronto
  ready();
  // Attende il segnale di start
  pause();
  start();
}
