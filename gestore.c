#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#ifndef GESTORE_H
#include "gestore.h"
#endif

FILE* urandom;
/*
    File contenuto in /dev/urandom
    utilizzato per leggere random bytes.
*/


people* a_people; // Puntatore alla lista di processi attivi di tipo A
people* b_people; // Puntatore alla lista di processi attivi di tipo B
person* best_genoma;
person* longest_name;

pid_t a_matching;
pid_t b_matching;

// Persone totali create
unsigned int total;
// Accoppiamenti totali
unsigned int matches;
char* a_path;
char* b_path;
struct timeval start_time;

int shmid;
void* shmptr;

int msq_match;
int msq_start;
int msgsize;



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
  int shm_size = sizeof(int) + (sizeof(a_person) * SHM_LENGTH);
  if ((shmid = shmget(SHM_KEY,shm_size,flag)) == -1) {
    printf("Failed to create shared memory segment.\n");
    raise(SIGTERM);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    printf("Failed to attach shared memory segment.\n");
    raise(SIGTERM);
  }
}


/* Rimuove la memoria condivisa */
void shm_destroy() {
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach shared memory segment.\n");
    raise(SIGTERM);
  }
  if (shmctl(shmid,IPC_RMID,NULL) == -1) {
    printf("Failed to remove shared memory segment.\n");
    raise(SIGTERM);
  }
}


/* Stampa il tempo trascorso dall'inizio della simulazione
con una precisione di microsecondi */
void print_time() {
  double start = start_time.tv_sec+((double)start_time.tv_usec/1e6);
  double now;
  struct timeval x;
  gettimeofday(&x,NULL);
  unsigned int seconds = x.tv_sec;
  unsigned int microseconds = x.tv_usec;
  now = seconds+((double)microseconds/1e6);
  printf("Tempo trascorso: %lf sec\n",now-start);
}


/* Stampa le info della simulazione */
void print_info(int sig) {
  printf("\n\n###  TIPO A  ###\n\n");
  print_people(a_people);
  printf("###  TIPO B  ###\n\n");
  print_people(b_people);
  printf("Individui totali creati: %u\n",total);
  printf("Accoppiamenti: %u\n",matches);
  print_time();
  printf("\n\n");
  alarm(15);
}


/* Ritorna il processo con genoma minimo o NULL nel caso
speciale in cui gli individui esistenti siano 2 e si stanno
accoppiando */
person* choose_victim() {
  node* n = a_people->first;
  person* p = n->elem;
  unsigned long min = p->genoma;
  for (int i = 0;i < a_people->length;i++) {
    if (n->elem->genoma < min && n->elem->pid != a_matching) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  n = b_people->first;
  for (int i = 0;i < b_people->length;i++) {
    if (n->elem->genoma < min && n->elem->pid != a_matching) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  if (p->pid == a_matching || p->pid == b_matching) {
    return NULL;
  } else {
    return p;
  }
}

/*
Immette un nuovo individuo nella popolazione.
Se nome e' diverso da NULL l'individuo e' generato da un genitore,
altrimenti avra' le caratteristiche iniziali. La funzione provvede
a non estinguere nessuno dei due tipi.
*/
person* spawn_new_person(char* nome, unsigned long mcd) {
  person* p;
  int type;
  // forza il tipo se necessario
  if (a_people->length == 0 && b_people->length == 0) {
    type = -1;
  } else if (b_people->length == 0) {
    type = B;
  } else if (a_people->length == 0) {
    type = A;
  } else {
    type = -1;
  }
  // crea la persona
  if (nome != NULL) {
    p = create_person(nome,mcd,type);
  } else {
    p = create_rand_person(type);
  }
  // inizializza l'id
  p->id = total++;
  // genera il processo
  gen_child(p);
  return p;
}


/* Riceve una struttura person, crea un processo effettivo
e aggiunge la persona nella memoria condivisa se e' di tipo A */
void gen_child(person* p) {
  // scrivo genoma e id in una stringa da passare come argomento dell'execl
  char* genoma = malloc(64);
  sprintf(genoma,"%lu",p->genoma);
  char* id = malloc(64);
  sprintf(id,"%u",p->id);
  // fork
  pid_t child = fork();
  switch(child) {
    case -1: { // errore
      printf("Failed forking.");
      raise(SIGTERM);
    }
    case 0: { // figlio
      if (p->tipo == A) {
        execl(a_path,"a",p->nome,genoma,NULL);
        printf("Execl not working\n");
      } else {
        execl(b_path,"b",p->nome,genoma,id,NULL);
        printf("Execl not working\n");
      }
    }
    default: { // padre
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


/* Riceve i pid dei processi accoppiati, pulisce le loro strutture e
crea gli individui figli */
void accoppia(int a, int b) {
  // aumenta il numero di accoppiamenti totali
  matches++;
  // toglie A dalla memoria condivisa
  shm_pop(shmptr,a);
  // esegue le wait
  waitpid(a,NULL,0);
  waitpid(b,NULL,0);
  // svuota i messaggi
  empty_queue(a,A);
  empty_queue(b,B);
  // prende le persone dalla lista
  person* old_a = get_person(a_people,a);
  person* old_b = get_person(b_people,b);
  unsigned long mcd_v = mcd(old_a->genoma,old_b->genoma);
  // le rimuove dalla lista
  pop_person(a_people,a);
  pop_person(b_people,b);
  // crea i nuovi individui
  person* new_1 = spawn_new_person(old_a->nome,mcd_v);
  person* new_2 = spawn_new_person(old_b->nome,mcd_v);
  // riceve il messaggio di pronto
  ready_receive(new_1);
  ready_receive(new_2);
  // libera la memoria delle persone terminate
  delete_person(old_a);
  delete_person(old_b);
  // Segnala di iniziare il ciclo di vita
  segnala_start(new_1);
  segnala_start(new_2);
  // resetta le variabili di matching
  a_matching = 0;
  b_matching = 0;
  printf("Match A: %d\tB: %d\n",a,b);
}


/* Svuota la coda di messaggi di un processo terminato.
Se di tipo A, rifiuta tutte le richieste pendenti
*/
void empty_queue(pid_t pid, int type) {
  if (type == A) {
    message r;
    message s;
    errno = 0;
    int i = 0;
    while (msgrcv(msq_match,&r,msgsize,pid,IPC_NOWAIT) != -1  || errno == EINTR) {
      if (i > 1000) {
        printf("empty_queue bug type A\n");
        break;
      }
      // rifiuta le richieste pendenti
      s.data = 0;
      s.mtype = r.pid;
      s.pid = pid;
      while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
      i++;
    }
  } else {
    errno = 0;
    while (msgrcv(msq_match,NULL,msgsize,pid,IPC_NOWAIT) != -1 || errno == EINTR) continue;
  }

}

void print_message(message* m) {
  printf("Pid: %d\tmtype: %lu\tpartner: %d\n\n",m->pid,m->mtype,m->partner);
}


/* Attende messaggi di accoppiamento. Vengono usati i while nel caso
in cui un segnale blocchi la ricezione del messaggio */
void wait_for_messages() {
  message a;
  message b;
  while (1) {
    // Riceve il messaggio di un processo A
    errno = 0;
    while (msgrcv(msq_match,&a,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
    // Segnala a birth_death che i processi sono in fase di matching
    a_matching = a.pid;
    b_matching = a.partner;
    // Da' tempo al messaggio di B di arrivare
    usleep(5000);
    // Riceve il messaggio del processo B con IPC_NOWAIT
    errno = 0;
    while (msgrcv(msq_match,&b,msgsize,a.partner,IPC_NOWAIT) == -1 && errno == EINTR) {
      errno = 0;
    }
    switch (errno) {
      case 0: { // Il messaggio e' stato ricevuto
        // controlla che i processi siano ancora vivi
        if (kill(a.pid,0) + kill(b.pid,0) < 0) { // uno e' terminato
          // Uno dei due processi e' stato ucciso da birth_death
          // prima della ricezione del messaggio di A
          // Restarta il processo in pausa.
          kill(a.pid,SIGUSR1);
          kill(b.pid,SIGUSR1);
        } else {  // sono entrambi vivi
          // segnala ai processi di terminare
          kill(b.pid,SIGTERM);
          kill(a.pid,SIGTERM);
          accoppia(a.pid,b.pid);
        }
        break;
      } case ENOMSG: {
        // Il processo B e' terminato prima di sapere del consenso di A
        // segnala al processo A di continuare
        print_error();
        kill(a.pid,SIGUSR1);
        break;
      } default: {
        print_error();
        raise(SIGTERM);
        break;
      }
    }
  }
}


/* Termina un processo e ne crea uno nuovo ogni BIRTH_DEATH secondi */
void birth_death(int sig) {
  if (a_people->length+b_people->length == 2) {
    printf("Birth Death not possible!\n");
    return;
  }
  // resetta il segnale SIGALRM
  alarm(BIRTH_DEATH);
  // sceglie una vittima
  person* victim;
  while ((victim = choose_victim()) == NULL) continue;
  printf("Victim:\n");
  print_person(victim);
  pid_t pid = victim->pid;
  if (victim->tipo == A) {
    // rimuove la persona dalla memoria condivisa
    shm_pop(shmptr,pid);
    // rimuove la persona dalla lista di persone di quel tipo
    pop_person(a_people,pid);
    empty_queue(pid,A);
  } else {
    pop_person(b_people,pid);
    empty_queue(pid,B);
  }
  // manda il segnale di terminazione al processo
  if (kill(pid, SIGTERM) == -1) {
    printf("Segnale di terminazione fallito!\n");
    raise(SIGTERM);
  }
  // esegue la wait
  int status;
  waitpid(pid,&status,0);
  if (status != EXIT_SUCCESS) {
    printf("%d did not exited with success.\n",pid);
  }
  // Crea un nuovo individuo
  person* new = spawn_new_person(NULL,0);
  // Attende che il processo sia pronto
  ready_receive(new);
  // Segnala di iniziare
  segnala_start(new);
  // Stampa le informazioni della simulazione
  print_info(0);;
}


/* Uccide tutti i processi figli e esegue le wait su di essi */
void kill_all() {
  people_for_each(a_people,kill_person);
  people_for_each(a_people,kill_person);
  while (wait(NULL) != -1) continue;
  if (errno != ECHILD) {// errore inatteso
    printf("kill_all error\n");
    print_error();
  }
}


/* Manda il segnale di terminazione alla persona p */
void kill_person(person* p) {
  kill(p->pid,SIGTERM);
}


/* Manda il segnale di start alla persona p */
void segnala_start(person* p) {
  if (kill(p->pid,SIGUSR1) == -1) {
    printf("segnala_start error\n");
    print_error();
  }
}


/* Riceve il messaggio di pronto della persona p */
void ready_receive(person* p) {
  message m;
  if (msgrcv(msq_start,&m,msgsize,p->pid,0) == -1 && errno != EINTR) {
    print_error();
    raise(SIGTERM);
  }
}

/* Gestisce i segnali con sigaction */
void set_signals() {
  struct sigaction s_quit;
  struct sigaction s_birth_death;
  // assegna gli handler
  s_quit.sa_handler = quit;
  s_birth_death.sa_handler = print_info;
  // quit non viene interrotto da nessun segnale
  sigfillset(&s_quit.sa_mask);
  // birth_death puo' essere interrotto dagli altri segnali
  sigemptyset(&s_birth_death.sa_mask);
  // handler per i segnali di terminazione
  sigaction(SIGINT,&s_quit,NULL);
  sigaction(SIGTERM,&s_quit,NULL);
  // Il birth-death avviene tramite il segnale SIGALRM
  sigaction(SIGALRM,&s_birth_death,NULL);
}

/* Inizia la simulazione */
void start() {
  for (int i = 0;i < INIT_PEOPLE;i++) {
    spawn_new_person(NULL,0);
  }
  people_for_each(a_people,ready_receive);
  people_for_each(b_people,ready_receive);
  print_info(0);
  people_for_each(a_people,segnala_start);
  people_for_each(b_people,segnala_start);
  wait_for_messages();
}

/* Handler dei segnali di terminazione.
Uccide i figli, cancella le strutture IPC e stampa le informazioni finali */
void quit(int sig) {
  kill_all();
  print_info(0);
  // cancella la memoria condivisa
  shm_destroy();
  // cancella la coda di messaggi
  msgctl(msq_match,IPC_RMID,NULL);
  msgctl(msq_start,IPC_RMID,NULL);
  // chiude il file urandom
  fclose(urandom);
  // exit
  exit(EXIT_SUCCESS);
}


/* Inizializza le strutture IPC, le variabili di gestione,
gli handler dei segnali etc. */
void init() {
  // gestione segnali
  set_signals();
  // Inizializza le variabili a 0
  a_matching = 0;
  b_matching = 0;
  total = 0;
  matches = 0;
  // Definisce il tempo iniziale
  gettimeofday(&start_time,NULL);
  // Apre il file urandom
  urandom = fopen("/dev/urandom", "r");
  // Crea le code di messaggi
  if ((msq_match = msgget(MSG_MATCH,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
    printf("Failed to create message queue.\n");
  }
  if ((msq_start = msgget(MSG_START,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
    printf("Failed to create message queue.\n");
  }
  msgsize = sizeof(message)-sizeof(long);
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
}


int main() {
  init();
  start();
}
