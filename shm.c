#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef SHM_H
#include "shm.h"
#endif


/* Aggiorna la memoria condivisa con il nuovo processo A,
 andando a sostituire un vecchio processo con stesso pid
 oppure aggiungendo un nuovo elemento all'array */
void shm_push(void* shmptr, person* p) {
  unsigned int* length = shmptr;
  a_person* a = shmptr+sizeof(unsigned int);
  int found = 0;
  int i = 0;
  while (i < *length && !found) {
    found = a[i].pid == p->pid;
    i++;
  }
  if (found) { // Trovato un vecchio processo
    if (a[--i].valid) { // Esiste un altro processo con stesso pid,
      printf("Errore! Non possono esistere due processi attivi con stesso pid.\n");
      exit(EXIT_FAILURE);
    }
    a[i].genoma = p->genoma;
    a[i].pid = p->pid;
    a[i].valid = 1;
  } else { // Non esiste in memoria un processo con quel pid, quindi si aggiunge un nuovo elemento
    if (*length == SHM_LENGTH) {
      // Si e' raggiunto il limite di spazio della memoria condivisa
      printf("Non c'e' piu spazio nella memoria condivisa, aumentare SHM_LENGTH\n");
      exit(EXIT_FAILURE);
    }
    a[*length].valid = 1;
    a[*length].genoma = p->genoma;
    a[*length].pid = p->pid;
    (*length)++;
  }
}


/* Pone a 0 il byte valid del processo A cercando il pid */
void shm_pop(void* shmptr, pid_t pid) {
  unsigned int* length = shmptr;
  a_person* a = shmptr+sizeof(unsigned int);
  int i = 0;
  int found = 0;
  while (i < *length && !found) {
    found = a[i].pid == pid;
    i++;
  }
  if (found) {
    if (!a[--i].valid) {
      printf("Processo A con pid %d e' gia' invalidato!\n",pid);
      exit(EXIT_FAILURE);
    }
    // Ha trovato l'elemento da invalidare
    a[i].valid = 0;
  } else {
    // L'elemento non esiste
    printf("Processo A con pid %d non esiste nella memoria condivisa!\n",pid);
  }
}


/* Stampa tutti i processi A validi */
void print_valid_shm(void* shmptr) {
  unsigned int* length = shmptr;
  a_person* a = shmptr+sizeof(unsigned int);
  printf("GENOMA\tPID\n\n");
  for (int i = 0; i < *length;i++) {
    if (a[i].valid) {
      printf("%lu\t%d\n",a[i].genoma,a[i].pid);
    }
  }
  printf("\n");
}


/* Stampa tutti i processi A nella memoria condivisa */
void print_all_shm(void* shmptr) {
  unsigned int* length = shmptr;
  a_person* a = shmptr+sizeof(unsigned int);
  printf("VALID\tGENOMA\tPID\n\n");
  for (int i = 0; i < *length;i++) {
    printf("%s\t%lu\t%d\n",(a[i].valid ? "TRUE" : "FALSE"),a[i].genoma,a[i].pid);
  }
  printf("\n");
}


a_person* get_best_match(void* shmptr, unsigned long genoma) {
  unsigned int* length = shmptr;
  a_person* a = shmptr+sizeof(unsigned int);
  // TODO
}
