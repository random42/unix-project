#ifndef SHM_H
#define SHM_H
#include "people.h"
#define SHM_LENGTH 2048
#define SHM_KEY 1234

// Struttura per le persone A nella memoria condivisa
typedef struct {
  unsigned int id;
  unsigned long genoma;
  pid_t pid;
  char valid; // byte di validita' del processo
} a_person;

void shm_push(void* shmptr, person* p);
void shm_pop(void* shmptr, pid_t pid);
void print_valid_shm(void* shmptr);
void print_all_shm(void* shmptr);

#endif
