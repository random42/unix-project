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


FILE* urandom;
/*
    File contenuto in /dev/urandom
    utilizzato per leggere random bytes.
*/

person** a_children; // Puntatore ai processi di tipo A
person** b_children; // Puntatore ai processi di tipo B
person* best_genoma;
person* longest_name;
person* shmptr;
unsigned int total;
int a_length;
int b_length;
char* a_path;
char* b_path;
time_t start_time;

int shmid;
int msqid;


/* Crea la memoria condivisa con i processi di tipo B */
void shm_init() {
  int flag = IPC_CREAT | IPC_EXCL | 0666;
  if ((shmid = shmget(SHM_KEY,sizeof(person)*MAX_A+200*MAX_A,flag)) == -1) {
    printf("Failed to create shared memory segment.\n");
    exit(1);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (person*)-1) {
    printf("Failed to attach shared memory segment.\n");
    exit(1);
  }
}

/* Rimuove la memoria condivisa */
void shm_destroy() {
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    exit(1);
  }
  if (shmctl(shmid,IPC_RMID,NULL) == -1) {
    printf("Failed to remove shared memory segment.\n");
    exit(1);
  }
}

/* Genera un tipo (A,B) random */
type_t random_type() {
  short num;
  fread(&num, sizeof(short), 1, urandom);
  return abs(num%2);
}

/* Stampa un array di persone arr di lunghezza length  */
void print_people(person** arr,int length) {
  printf("TIPO\tNOME\tGENOMA\tPID\n");
  int i = 0;
  while (i < length) {
    printf("%s\t%s\t%lu\t%d\n",(arr[i]->tipo == 0 ? "A":"B"),arr[i]->nome,arr[i]->genoma,arr[i]->pid);
    i++;
  }
}

/* Stampa le info della simulazione */
void print_info() {
  printf("TEMPO\tTOTALI\tATTIVI\tA\tB\n");
  printf("%d\t%d\t%d\t%d\t%d\n",(int)difftime(time(NULL),start_time),total,a_length+b_length,a_length,b_length);
}

person* create_person(char* name, int mcd, int type) {
  person* r = malloc(sizeof(person));
  r->nome = malloc(strlen(name)+1);
  strcpy(r->nome,name);
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  *(r->nome+strlen(name)) = (n % 26) + 65;
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  r->genoma = (gen % GENES) + mcd;
  switch (type) {
    case 0: {
      r->tipo = A;
      break;
    }
    case 1: {
      r->tipo = B;
      break;
    }
    default: {
      r->tipo = random_type();
      break;
    }
  }
  return r;
}

/* Genera una struttura person random */
person* create_rand_person() {
  person* r = malloc(sizeof(person));
  r->nome = malloc(1);
  r->tipo = random_type();
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  r->genoma = (gen % GENES)+2;
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  *(r->nome) = (n % 26) + 65;
  r->pid = 0;
  return r;
}

/* Inizializza la memoria condivisa e apre il file urandom */
void init() {
  start_time = time(NULL);
  urandom = fopen("/dev/urandom", "r");
  msqid = msgget(MSG_KEY,IPC_CREAT | IPC_EXCL);
  shm_init();
  char* dir = getenv("PWD");
  a_path = malloc(strlen(dir)+6);
  b_path = malloc(strlen(dir)+6);
  sprintf(a_path,"%s/bin/a",dir);
  sprintf(b_path,"%s/bin/b",dir);
  a_children = malloc(sizeof(person*)*);
}


// int birth_death() {
//   choose_victim();
//   if (victim.tipo == A) {
//     rimuovi_dalla_memoria();
//   }
//   gen_child();
//   print_info();
// }

/* Genera num persone random */
person** create_people(int num) {
  person** p = malloc(sizeof(person*)*num);
  int i = 0;
  while (i < num) {
    p[i++] = create_rand_person();
  }
  return p;
}

/* Riceve una struttura person crea un processo effettivo */
void gen_child(person* p) {
  char* genoma = malloc(64);
  sprintf(genoma,"%lu",p->genoma);
  pid_t child = fork();
  switch(child) {
    case -1: {
      printf("Failed forking.");
      exit(1);
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
      p->pid = child;
      print_people(&p,1);
    }
  }
}


void quit() {
  wait(NULL);
  shm_destroy();
  msgctl(msqid,IPC_RMID,NULL);
  fclose(urandom);
}



int main() {
  // atexit(quit);
  // init();
}
