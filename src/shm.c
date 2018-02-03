#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "header.h"
#include "shm.h"

extern char* stack[];
extern int stack_length;
extern int debug_info;

/* Inserisce nella memoria condivisa il nuovo processo A,
 andandolo a sostituire al primo processo non valido (gia' terminato)
 oppure aggiungendo un nuovo elemento all'array */
void shm_push(void* shmptr, person* p) {
  add_func("shm_push");
  int length = *(int*)shmptr;
  a_person* a = shmptr+sizeof(int);
  int found = 0;
  int i = 0;
  // cerca uno spazio riutilizzabile
  while (i < length && !found) {
    found = a[i].valid == 0;
    i++;
  }
  if (found) { // esiste
    a[--i].id = p->id;
    a[i].pid = p->pid;
    a[i].genoma = p->genoma;
    a[i].valid = 1;
  } else {  // non esiste, quindi aggiunge un nuovo elemento
    a[length].id = p->id;
    a[length].pid = p->pid;
    a[length].genoma = p->genoma;
    a[length].valid = 1;
    (*(int*)shmptr)++;
  }
  rm_func();
}


/* Pone a 0 il byte valid del processo A cercando il pid */
void shm_pop(void* shmptr, pid_t pid) {
  add_func("shm_pop");
  int length = *(int*)shmptr;
  a_person* a = shmptr+sizeof(int);
  int i = 0;
  int found = 0;
  while (i < length && !found) {
    found = a[i].pid == pid;
    i++;
  }
  if (found) {
    // Ha trovato l'elemento da invalidare
    a[--i].valid = 0;
  } else {
    // L'elemento non esiste
    printf("Processo A con pid %d non esiste nella memoria condivisa!\n",pid);
    raise(SIGTERM);
  }
  rm_func();
}


/* Stampa tutti i processi A validi */
void print_valid_shm(void* shmptr) {
  add_func("print_valid_shm");
  int length = *(int*)shmptr;
  a_person* a = shmptr+sizeof(int);
  printf("\nID\tGENOMA\tPID\n\n");
  for (int i = 0; i < length;i++) {
    if (a[i].valid) {
      printf("%u\t%lu\t%d\n",a[i].id,a[i].genoma,a[i].pid);
    }
  }
  printf("\n");
  rm_func();
}


/* Stampa tutti i processi A nella memoria condivisa */
void print_all_shm(void* shmptr) {
  add_func("print_all_shm");
  int length = *(int*)shmptr;
  a_person* a = shmptr+sizeof(int);
  printf("\nVALID\tID\tGENOMA\tPID\n\n");
  for (int i = 0; i < length;i++) {
    printf("%s\t%u\t%lu\t%d\n",(a[i].valid ? "TRUE" : "FALSE"),a[i].id,a[i].genoma,a[i].pid);
  }
  printf("\n");
  rm_func();
}
