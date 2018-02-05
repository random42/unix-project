#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include "header.h"
#include "sem.h"

extern int sem_start;
extern int sem_match;
extern unsigned int INIT_PEOPLE;
extern char* stack[];
extern int stack_length;
extern int debug_info;

void sem_create() {
  add_func("sem_create");
  if ((sem_start = semget(SEM_START,INIT_PEOPLE, IPC_CREAT /* | IPC_EXCL */| S_IRUSR | S_IWUSR )) == -1) {
    printf("sem_start create failed\n");
    print_error();
    raise(SIGTERM);
  }
  if ((sem_match = semget(SEM_MATCH,INIT_PEOPLE, IPC_CREAT /* | IPC_EXCL */| S_IRUSR | S_IWUSR )) == -1) {
    printf("sem_match create failed\n");
    print_error();
    raise(SIGTERM);
  }
  unsigned short start[INIT_PEOPLE];
  for (int i = 0;i < INIT_PEOPLE;i++) {
    start[i] = 1;
  }
  if (semctl(sem_start,0,SETALL,start) == -1) {
    printf("semctl error\n");
    print_error();
    raise(SIGTERM);
  }
  for (int i = 0;i < INIT_PEOPLE;i++) {
    start[i] = 0;
  }
  if (semctl(sem_match,0,SETALL,start) == -1) {
    printf("semctl error\n");
    print_error();
    raise(SIGTERM);
  }
  rm_func();
}

void sem_destroy() {
  add_func("sem_destroy");
  if (semctl(sem_match,0,IPC_RMID) == -1) {
    printf("Failed destroying sem_match\n");
    print_error();
  }
  if (semctl(sem_start,0,IPC_RMID) == -1) {
    printf("Failed destroying sem_match\n");
    print_error();
  }
  rm_func();
}

void sem_init() {
  add_func("sem_init");
  if ((sem_start = semget(SEM_START,0,0)) == -1) {
    printf("sem_start init failed\n");
    print_error();
    raise(SIGTERM);
  }
  if ((sem_match = semget(SEM_MATCH,0,0)) == -1) {
    printf("sem_match init failed\n");
    print_error();
    raise(SIGTERM);
  }
  rm_func();
}

void print_sem_match() {
  add_func("print_sem_match");
  unsigned short array[INIT_PEOPLE];
  if (semctl(sem_match,0, GETALL, array) == -1) {
    printf("semctl error\n");
    print_error();
    raise(SIGTERM);
  }
  printf("sem_match: {");
  for (int i = 0;i < INIT_PEOPLE;i++) {
    printf("%d,",array[i]);
  }
  printf("}\n");
  rm_func();
}

void print_sem_start() {
  add_func("print_sem_start");
  unsigned short array[INIT_PEOPLE];
  if (semctl(sem_start,0, GETALL, array) == -1) {
    printf("semctl error\n");
    print_error();
    raise(SIGTERM);
  }
  printf("sem_start: {");
  for (int i = 0;i < INIT_PEOPLE;i++) {
    printf("%d,",array[i]);
  }
  printf("}\n");
  rm_func();
}

void wait_match(short num) {
  struct sembuf s;
  s.sem_num = num;
  s.sem_op = 0;
  while (semop(sem_match,&s,1) == -1 && errno == EINTR) continue;
}

void wait_start(short num) {
  struct sembuf s;
  s.sem_num = num;
  s.sem_op = 0;
  while (semop(sem_start,&s,1) == -1 && errno == EINTR) continue;
}

void add_match(short num, short op) {
  add_func("add_match");
  struct sembuf s;
  s.sem_num = num;
  s.sem_op = op;
  while (semop(sem_match,&s,1) == -1 && errno == EINTR) continue;
  rm_func();
}

void add_start(short num, short op) {
  add_func("add_start");
  struct sembuf s;
  s.sem_num = num;
  s.sem_op = op;
  while (semop(sem_start,&s,1) == -1 && errno == EINTR) continue;
  rm_func();
}

void set_all(int id, short num) {
  unsigned short a[INIT_PEOPLE];
  for (short i = 0;i < INIT_PEOPLE;i++) {
    a[i] = num;
  }
  while (semctl(id,0,SETALL,a) == -1 && errno == EINTR) continue;
}

void set_one(int id, short sem, short num) {
  while (semctl(id,sem,SETVAL,num) == -1 && errno == EINTR) continue;
}
