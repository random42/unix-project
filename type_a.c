#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "header.h"
#include "child.h"
#include "type_a.h"



// Proprie caratteristiche
unsigned long genoma;
char* nome;

// mcd target
unsigned long target;

// Numero di contatti rifiutati nel corrente target
unsigned long contacts;

// Divisori del proprio genoma
unsigned long* divisori;
int div_length;

int msq_match;
int msq_start;
int msq_contact;
int msgsize;

char* debug_func;
int debug_info;

void abbassa_target() {
  debug_func("abbassa_target");
  int i = 0;
  while (i < div_length && target != divisori[i]) i++;
  if (target > 1) {
    target = divisori[i+1];
  }
}


// registra l'id in first_contact o sec_contact
void push_contact(unsigned int id) {
  debug_func("push_contact");
  contacts++;
  if (contacts >= INIT_PEOPLE) {
    abbassa_target();
    contacts = 0;
  }
}

// accetta il processo B e si accoppia
void accetta(pid_t pid) {
  debug_func("accetta");
  message s;
  s.data = 1;
  s.mtype = pid;
  s.pid = getpid();
  s.partner = pid;
  // Messaggio di assenso al processo B
  while (msgsnd(msq_contact,&s,msgsize,0) == -1 && errno == EINTR) continue;
  // Attende il messaggio di conferma di B
  message r;
  while (msgrcv(msq_match,&r,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
  if (r.data) { // B ha confermato
    // Messaggio per il gestore
    s.mtype = getppid();
    while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
  } else { // B e' stato ucciso quindi il gestore ha risposto al suo posto
    return;
  }
  debug_func("Pause 2");
  pause();
  printf("A %d riprende esecuzione\n",getpid());
}

// rifiuta il processo B
void rifiuta(pid_t pid) {
  debug_func("rifiuta");
  message s;
  s.data = 0;
  s.mtype = pid;
  s.pid = getpid();
  while (msgsnd(msq_contact,&s,msgsize,0) == -1 && errno == EINTR) continue;
}

void ascolta() {
  message r;
  message s;
  while (1) {
    debug_func("ascolta");
    // ricevo il messaggio
    debug_info = 1;
    while (msgrcv(msq_contact,&r,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
    if (mcd(genoma,r.genoma) >= target) { // l'mcd corrisponde al target
      debug_info = 2;
      accetta(r.pid);
    } else {
      debug_info = 3;
      rifiuta(r.pid);
      debug_info = 4;
      push_contact(r.id);
    }
  }
}

void debug(int sig) {
  printf("%d {type: A, function: %s, info: %d, target: %lu, genoma: %lu, contacts: %lu}\n",getpid(),debug_func,debug_info,target,genoma,contacts);
  quit(0);
}

void init() {
  set_signals(quit,debug);
  debug_func = malloc(64);
  // le variabili
  contacts = 0;
  // le code di messaggi
  msq_init();
  // cerca i divisori
  trova_divisori();
  // target iniziale = genoma
  target = divisori[0];
}

void quit(int sig) {
  exit(EXIT_SUCCESS);
}

void start() {
  alarm(5);
  ascolta();
}

int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  init();
  // Segnala al gestore di essere pronto
  ready();
  // Attende il segnale di start
  debug_func("Pause 1");
  pause();
  start();
}
