#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include "header.h"
#include "shm.h"

pid_t pid;
unsigned long genoma;
char* nome;

int msqid;
int msgsize;

int shmid;
void* shmptr;

/* Aquisisce la memoria condivisa con il gestore */
void shm_init() {
  int flag = 0600;
  int size = sizeof(unsigned int) + (sizeof(a_person) * SHM_LENGTH);
  if ((shmid = shmget(SHM_KEY,size,flag)) == -1) {
    printf("Failed to get shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    printf("Failed to attach shared memory segment.\n");
    exit(EXIT_FAILURE);
  }
}

/* Esegue il detach della memoria condivisa */
void shm_detach() {
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    exit(EXIT_FAILURE);
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

void cerca() {
  unsigned int max_mcd = 0;
  for ()
}

void contatta(pid_t pid) {

}

void accoppia(pid_t a_pid) {

}

void init() {
  pid = getpid();
  if ((msqid = msgget(MSG_KEY,0)) == -1) {
    printf("Failed to get message queue.\n");
    exit(EXIT_FAILURE);
  }
  msgsize = sizeof(message)-sizeof(long);
  shm_init();
}

void quit() {
  shm_detach();
  exit(EXIT_SUCCESS);
}


int main(int argc, char* argv[]) {
  nome = argv[1];
  genoma = strtoul(argv[2],NULL,10);
  init();
  print_all_a(shmptr);
  //signal(SIGTERM,quit);
  quit();
}
