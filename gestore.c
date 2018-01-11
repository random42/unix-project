#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <string.h>
#ifndef HEADER_H
#include "header.h"
#endif
#ifndef SHM_H
#include "shm.h"
#endif
#ifndef PEOPLE_H
#include "people.h"
#endif

extern unsigned int errno;

FILE* urandom;
/*
    File contenuto in /dev/urandom
    utilizzato per leggere random bytes.
*/


people* a_people; // Puntatore alla lista di processi attivi di tipo A
people* b_people; // Puntatore alla lista di processi attivi di tipo B
person* best_genoma;
person* longest_name;

unsigned int total;
char* a_path;
char* b_path;
time_t start_time;

int shmid;
void* shmptr;

int msqid;


/* Guess what */
unsigned long mcd(unsigned long a, unsigned long b) {
  unsigned long r;
  while (a % b != 0) {
    r = a%b;
    a = b;
    b = r;
  }
  return b;
}

/* Crea la memoria condivisa con i processi di tipo B */
void shm_init() {
  int flag = IPC_CREAT /* | IPC_EXCL */ | 0600;
  int size = sizeof(unsigned int) + (sizeof(a_person) * SHM_LENGTH);
  if ((shmid = shmget(SHM_KEY,size,flag)) == -1) {
    printf("Failed to create shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    printf("Failed to attach shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
}


/* Rimuove la memoria condivisa */
void shm_destroy() {
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
  if (shmctl(shmid,IPC_RMID,NULL) == -1) {
    printf("Failed to remove shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
}


/* Stampa le info della simulazione */
void print_info() {
  //TODO
}


/* Sceglie il processo con genoma minimo */
person* choose_victim() {
  node* n = a_people->first;
  person* p = n->elem;
  unsigned long min = p->genoma;
  for (int i = 0;i < a_people->length;i++) {
    if (n->elem->genoma < min) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  n = b_people->first;
  for (int i = 0;i < b_people->length;i++) {
    if (n->elem->genoma < min) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  return p;
}


/* Riceve una struttura person e crea un processo effettivo */
void gen_child(person* p) {
  char* genoma = malloc(64);
  sprintf(genoma,"%lu",p->genoma);
  pid_t child = fork();
  switch(child) {
    case -1: {
      printf("Failed forking.");
      exit(EXIT_FAILURE);
    }
    case 0: {
      if (p->tipo == A) {
        execl(a_path,"a",p->nome,genoma,NULL);
        printf("Execl not working\n");
      } else {
        execl(b_path,"b",p->nome,genoma,NULL);
        printf("Execl not working\n");
      }
    }
    default: {
      total++;
      p->pid = child;
      // Aggiungo il nuovo processo alla rispettiva lista di persone
      if (p->tipo == A) {
        push_person(a_people,p);
        // Aggiungo le informazioni nella memoria condivisa se il tipo e' A
        shm_push(shmptr,p);
      } else {
        push_person(b_people,p);
      }
    }
  }
}


/* Attende messaggi di accoppiamento */
void wait_for_messages() {
  //message m[2];
  message r;
  int size = sizeof(message)-sizeof(long);
}


/* Termina un processo e ne crea uno nuovo ogni BIRTH_DEATH secondi */
void birth_death(int signum) {
  // scelgo una vittima
  person* victim = choose_victim();
  pid_t pid = victim->pid;
  if (victim->tipo == A) {
    // rimuovo la persona dalla memoria condivisa
    shm_pop(shmptr,pid);
    // rimuovo la persona dalla lista di persone di quel tipo
    pop_person(a_people,pid);
  } else {
    pop_person(b_people,pid);
  }
  // mando il segnale di terminazione al processo
  if (kill(pid, SIGTERM) == -1) {
    printf("Segnale di terminazione fallito!\n");
    exit(EXIT_FAILURE);
  }
  person* new;
  // Creo una nuova persona e ne forzo il tipo nel caso non ne esistano
  if (a_people->length == 0) {
    new = create_rand_person(A);
  } else if (b_people->length == 0) {
    new = create_rand_person(B);
  } else {
    new = create_rand_person(-1);
  }
  // Creo il processo della nuova persona
  gen_child(new);
  // Stampo le informazioni della simulazione
  print_info();
  // Resetto il segnale SIGALRM
  alarm(BIRTH_DEATH);
}


/* Esegue una waitpid()*/
void wait_pid(type_t type, pid_t pid) {
  int status;
  waitpid(pid,&status,0);
  if (status != EXIT_SUCCESS) {
    printf("%d did not exited with success.\n",pid);
  } else {
    printf("%d exited with success.\n",pid);
  }
}


/* TODO */
void quit() {
  shm_destroy();
  msgctl(msqid,IPC_RMID,NULL);
  fclose(urandom);
}


/* TODO */
void init() {
  // Definisce il tempo iniziale
  start_time = time(NULL);
  // Apre il file urandom
  urandom = fopen("/dev/urandom", "r");
  // Crea la coda di messaggi
  if ((msqid = msgget(MSG_KEY,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
    printf("Failed to create message queue.\n");
  }
  // Inizializza la memoria condivisa
  shm_init();
  // Definisce i percorsi degli eseguibili
  char* dir = getenv("PWD");
  a_path = malloc(strlen(dir)+6);
  b_path = malloc(strlen(dir)+6);
  sprintf(a_path,"%s/bin/a",dir);
  sprintf(b_path,"%s/bin/b",dir);
  // Inizializza le liste di persone
  a_people = init_people();
  b_people = init_people();
  // Utilizza la funzione birth_death come handler del segnale SIGALRM
  // signal(SIGALRM,birth_death);
}


void set_random_pid(person* p) {
  unsigned int pid;
  fread(&pid,sizeof(unsigned int),1,urandom);
  p->pid = pid%10000;
}


int main() {
  atexit(quit);
  init();
  for (int i = 0;i < 10;i++) {
    person* p = create_rand_person(-1);
    p->pid = i;
    a_push(shmptr,p);
  }
  printf("Parent\n\n");
  print_all_a(shmptr);
  person* a = create_rand_person(B);
  gen_child(a);
  wait(NULL);
}
