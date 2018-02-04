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
#include "child.h"
#include "type_b.h"


// Proprie caratteristiche
unsigned long genoma;
char* nome;
unsigned int id;

unsigned int INIT_PEOPLE;

// pid del partner accoppiato
int partner;
int match_phase;

// Array degli ID dei processi A gia' contattati
unsigned int* black_list;
int black_list_length;

// Divisori del proprio genoma
unsigned long* divisori;
int div_length;

//mcd target
unsigned long target;

int msq_match;
int msq_start;
int msq_contact;
int msgsize;

int shmid;
void* shmptr;

char* stack[64];
int stack_length;
int debug_info;

/* Aquisisce la memoria condivisa con il gestore */
void shm_init() {
  add_func("shm_init");
  int flag = 0600;
  int size = sizeof(int) + (sizeof(a_person) * INIT_PEOPLE);
  if ((shmid = shmget(SHM_KEY,size,flag)) == -1) {
    exit(EXIT_FAILURE);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    exit(EXIT_FAILURE);
  }
  rm_func();
}

/* Esegue il detach della memoria condivisa */
void shm_detach() {
  add_func("shm_detach");
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
  rm_func();
}

int not_black_list(unsigned int id) {
  add_func("not_black_list");
  int found = 0;
  for (int i = 0; i < black_list_length && !found;i++) {
    if (id == black_list[i]) {
      found = 1;
    }
  }
  rm_func();
  return found-1;
}

/* Si accoppia ad A */
void accoppia(pid_t pid) {
  add_func("accoppia");
  partner = pid;
  message s;
  s.mtype = getpid();
  s.pid = getpid();
  s.partner = pid;
  // Manda il messaggio al gestore
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
  // Conferma ad A di aver mandato il messaggio
  s.mtype = pid;
  s.data = 1;
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
  match_phase = 0;
  // Attende il segnale di terminazione
  pause();
  partner = 0;
  printf("B %d riprende esecuzione\n",getpid());
  rm_func();
}

char contatta(pid_t pid) {
  add_func("contatta");
  message s;
  message r;
  s.mtype = pid;
  s.pid = getpid();
  s.genoma = genoma;
  s.id = id;
  while (msgsnd(msq_contact,&s,msgsize,0) == -1 && errno == EINTR) continue;
  match_phase = 1;
  while (msgrcv(msq_contact,&r,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
  match_phase = r.data ? 2 : 0;
  rm_func();
  return r.data;
}

void cerca_target() {
  add_func("cerca_target");
  int length = *(int*)shmptr;
  a_person* a = shmptr+sizeof(int);
  // contatta ogni A per cui l'MCD dei genomi sia >= al target
  for (int i = 0; i < length;i++) {
    if (a[i].valid && mcd(genoma,a[i].genoma) >= target && not_black_list(a[i].id)) {
      partner = a[i].pid;
      if (!contatta(partner)) { // se viene rifiutato aggiunge il processo A nella black_list
        black_list[black_list_length++] = a[i].id;
      } else { // altrimenti si accoppia
        accoppia(a[i].pid);
      }
    }
  }
  rm_func();
}

void cerca() {
  add_func("cerca");
  int i = 0;
  while (1) {
    if (i == div_length) { // resetta target = genoma e cancella la black_list
      i = 0;
      black_list_length = 0;
    }
    target = divisori[i++];
    cerca_target();
  }
  rm_func();
}

void init() {
  // inizializza la memoria per lo stack
  for (int i = 0;i < 64;i++) {
    stack[i] = malloc(64);
  }
  add_func("init");
  black_list = malloc(sizeof(int)*INIT_PEOPLE);
  set_signals(quit,debug);
  // le code di messaggi
  msq_init();
  // la memoria condivisa
  shm_init();
  // trova i divisori
  trova_divisori();
  rm_func();
}

void quit(int sig) {
  shm_detach();
  exit(EXIT_SUCCESS);
}

void start() {
  add_func("start");
  alarm(5);
  cerca();
  rm_func();
}

void debug(int sig) {
  printf("\n%s ",strsignal(sig));
  printf("%d {type: B, info: %d, target: %lu, genoma: %lu, partner: %d stack: [",getpid(),debug_info,target,genoma,partner);
  for (int i = 0; i < stack_length;i++) {
    printf("%s, ",stack[i]);
  }
  printf("]}\n");
  quit(sig);
}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  id = strtoul(argv[3],NULL,10);
  INIT_PEOPLE = strtoul(argv[4],NULL,10);
  init();
  // Segnala al gestore di essere pronto e attende lo start
  ready();
  start();
}
