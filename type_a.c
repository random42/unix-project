#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "header.h"
#include "type_a.h"



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
int msq_contact;
int msgsize;

char* debug_func;
int debug_info;

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
  debug_func("abbassa_target");
  int i = 0;
  while (i < div_length && target != divisors[i]) i++;
  if (target > 1) {
    target = divisors[i+1];
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
  // Messaggio per il gestore
  s.mtype = getppid();
  while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) {
    print_error();
    raise(SIGTERM);
  }
  // Aspetta il segnale di terminazione o quello di continue
  // se il processo B e' terminato prima tramite birth_death
  debug_func("Pause 2");
  pause();
  printf("A ricerca pid: %d\n",getpid());
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

void do_nothing(int sig){}

void msq_init() {
  if ((msq_match = msgget(MSG_MATCH,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  if ((msq_start = msgget(MSG_START,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
  if ((msq_contact = msgget(MSG_CONTACT,0)) == -1) {
    printf("Failed to get message queue. A\n");
    raise(SIGTERM);
  }
}

void set_signals() {
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

void debug(int sig) {
  printf("%d {type: A, function: %s, info: %d, target: %lu, genoma: %lu, contacts: %lu}\n",getpid(),debug_func,debug_info,target,genoma,contacts);
  quit(0);
}

void init() {
  set_signals();
  debug_func = malloc(64);
  // inizializza l'array di divisori
  divisors = malloc(sizeof(unsigned long) * (genoma > 60 ? genoma/5 : genoma));
  // le variabili
  contacts = 0;
  msgsize = sizeof(message)-sizeof(long);
  // le code di messaggi
  msq_init();
  // cerca i divisori
  find_divisors();
  // target iniziale = genoma
  target = divisors[0];
}

void quit(int sig) {
  exit(EXIT_SUCCESS);
}

void start() {
  alarm(5);
  ascolta();
}

void ready() {
  message m;
  m.mtype = getpid();
  while (msgsnd(msq_start,&m,msgsize,0) == -1 && errno == EINTR) continue;
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
