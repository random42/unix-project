#include <unistd.h>
#include <sys/wait.h>
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
#include "header.h"
#include "gestore.h"
#include "sem.h"
#include "shm.h"

FILE* urandom;
// File contenuto in /dev/urandom utilizzato per leggere random bytes.

unsigned int INIT_PEOPLE, SIM_TIME, BIRTH_DEATH;
unsigned long GENES;

people* a_people; // Puntatore alla lista di processi attivi di tipo A
people* b_people; // Puntatore alla lista di processi attivi di tipo B
person* best_genoma;
person* longest_name;

// pid dei processi in fase di accoppiamento
pid_t a_matching;
pid_t b_matching;

// messaggi di matching da parte di A e B
message a_mess;
message b_mess;

// Timer per birth_death
struct itimerval timer;

// Vale 1 in una sezione critica in cui birth_death non puo' avvenire
int not;

// Persone totali create
unsigned int total;
unsigned int total_a;
unsigned int total_b;
// Persone con genoma maggiore e nome piu' lungo
person* best_genoma;
person* longest_name;
// Accoppiamenti totali
unsigned int matches;
// Percorsi agli eseguibili di A e B
char* a_path;
char* b_path;
// Tempo iniziale
struct timeval start_time;

// semafori
int sem_start;
int sem_match;

// memoria condivisa
int shmid;
void* shmptr;

// code di messaggi
int msq_match;
int msq_contact;
int msgsize;

// debug
char* stack[64];
int stack_length;
int debug_info;


/* Massimo comune divisore */
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
  add_func("shm_init");
  int flag = IPC_CREAT /* | IPC_EXCL */ | 0600;
  int shm_size = sizeof(int) + (sizeof(a_person) * INIT_PEOPLE);
  if ((shmid = shmget(SHM_KEY,shm_size,flag)) == -1) {
    quit(SIGTERM);
  }
  if ((shmptr = shmat(shmid,NULL,0)) == (void*)-1) {
    quit(SIGTERM);
  }
  rm_func();
}


/* Rimuove la memoria condivisa */
void shm_destroy() {
  add_func("shm_destroy");
  if (shmdt(shmptr) == -1) {
    printf("Failed to detach SHM.\n");
    exit(EXIT_FAILURE);
  }
  if (shmctl(shmid,IPC_RMID,NULL) == -1) {
    printf("Failed to remove SHM.\n");
    exit(EXIT_FAILURE);
  }
  rm_func();
}


/* Crea le code di messaggi */
void msq_init() {
  add_func("msq_init");
  if ((msq_match = msgget(MSG_MATCH,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
    printf("Failed to create message queue.\n");
  }
  if ((msq_contact = msgget(MSG_CONTACT,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
    printf("Failed to create message queue.\n");
  }
  rm_func();
}


/* Ritorna il tempo trascorso dall'inizio della simulazione
con una precisione di microsecondi */
double elapsed_time() {
  add_func("elapsed_time");
  double start = start_time.tv_sec+((double)start_time.tv_usec/1e6);
  double now;
  struct timeval x;
  gettimeofday(&x,NULL);
  unsigned int seconds = x.tv_sec;
  unsigned int microseconds = x.tv_usec;
  now = seconds+((double)microseconds/1e6);
  rm_func();
  return now-start;
}


/* Stampa le info della simulazione */
void print_info() {
  add_func("print_info");
  double _time = elapsed_time();
  //printf("\n\n###  TIPO A  ###\n\n");
  //print_people(a_people);
  //printf("###  TIPO B  ###\n\n");
  //print_people(b_people);
  printf("\n\nIndividui totali creati: %u\n",total);
  printf("Accoppiamenti: %u\n",matches);
  printf("Tempo trascorso: %lf sec\n",_time);
  printf("Accoppiamenti/secondo: %.3lf\n",matches/_time);
  rm_func();
}


void print_final_info() {
  print_info();
  printf("Individui A creati: %u\n",total_a);
  printf("Individui B creati: %u\n",total_b);
  if (best_genoma) {
    printf("\nPersona con genoma maggiore:\n");
    printf("TIPO: %s\nID: %u\nPID: %d\nGENOMA: %lu\nNOME: %s\n",best_genoma->tipo == A ? "A" : "B",best_genoma->id,best_genoma->pid,best_genoma->genoma,best_genoma->nome);
  }
  if (longest_name) {
    printf("\nPersona con nome piu' lungo:\n");
    printf("TIPO: %s\nID: %u\nPID: %d\nGENOMA: %lu\nNOME: %s\n",longest_name->tipo == A ? "A" : "B",longest_name->id,longest_name->pid,longest_name->genoma,longest_name->nome);
  }
}


/* Sceglie una persona casuale che non sia in fase di match */
person* choose_victim() {
  add_func("choose_victim");
  int length = a_people->length + b_people->length;
  unsigned int index;
  fread(&index, sizeof(unsigned int), 1, urandom);
  index = index % length;
  person* p;
  node* n;
  if (index >= a_people->length) { // itera su B
    index -= a_people->length;
    n = b_people->first;
    for (int i = 0;i < index;i++) {
      n = n->next;
    }
  } else { // itera su A
    n = a_people->first;
    for (int i = 0;i < index;i++) {
      n = n->next;
    }
  }
  p = n->elem;
  if (p->pid == a_matching || p->pid == b_matching || wait_match(p->sem) == -1) {
    rm_func();
    return choose_victim();
  } else {
    debug_info = p->pid;
    rm_func();
    return p;
  }
}

/*
Immette un nuovo individuo nella popolazione.
Se nome e' diverso da NULL l'individuo e' generato da un genitore,
altrimenti avra' le caratteristiche iniziali. La funzione provvede
a non estinguere nessuno dei due tipi.
*/
person* spawn_new_person(char* nome, unsigned long mcd, short sem) {
  add_func("spawn_new_person");
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
  // numero del semaforo
  p->sem = sem;
  // genera il processo
  gen_child(p);
  // cambia best_genoma o longest_name se necessario
  if (total == 0) {
    best_genoma = p;
    longest_name = p;
  }
  if (strlen(p->nome) > strlen(longest_name->nome)) {
    longest_name = p;
  }
  if (p->genoma > best_genoma->genoma) {
    best_genoma = p;
  }
  // inizializza l'id
  p->id = total++;
  // Aggiungo il nuovo processo alla rispettiva lista di persone
  if (p->tipo == A) {
    total_a++;
    push_person(a_people,p);
    // Aggiungo le informazioni nella memoria condivisa se il tipo e' A
    shm_push(shmptr,p);
  } else {
    total_b++;
    push_person(b_people,p);
  }
  rm_func();
  return p;
}


/* Riceve una struttura person, crea un processo effettivo
e aggiunge la persona nella memoria condivisa se e' di tipo A */
void gen_child(person* p) {
  add_func("gen_child");
  // argomenti dell'exec
  char* genoma = malloc(64);
  sprintf(genoma,"%lu",p->genoma);
  char* id = malloc(64);
  sprintf(id,"%u",p->id);
  char* sem_num = malloc(64);
  sprintf(sem_num,"%hi",p->sem);
  char* people = malloc(64);
  sprintf(people,"%u",INIT_PEOPLE);
  // fork
  pid_t child = fork();
  switch(child) {
    case -1: { // errore
      printf("Failed forking.");
      raise(SIGTERM);
    }
    case 0: { // figlio
      if (p->tipo == A) {
        execl(a_path,"a",p->nome,genoma,id,sem_num,people,NULL);
        printf("Execl not working\n");
      } else {
        execl(b_path,"b",p->nome,genoma,id,sem_num,people,NULL);
        printf("Execl not working\n");
      }
    }
    default: { // padre
      p->pid = child;
    }
  }
  rm_func();
}


/* Riceve le persone da accoppiare, pulisce le loro strutture e
crea gli individui figli */
void accoppia(person* a, person* b) {
  add_func("accoppia");
  // aumenta il numero di accoppiamenti totali
  matches++;
  // termina le persone e pulisce le strutture
  termina_persona(a);
  termina_persona(b);
  // calcola l'mcd
  unsigned long mcd_v = mcd(a->genoma,b->genoma);
  // BIRTH_DEATH NON ESEGUIBILE
  not = 1;
  // crea i nuovi individui
  person* new_1 = spawn_new_person(a->nome,mcd_v,a->sem);
  person* new_2 = spawn_new_person(b->nome,mcd_v,b->sem);
  // attende che siano pronti
  wait_ready(new_1);
  wait_ready(new_2);
  // libera la memoria delle persone terminate
  delete_person(a);
  delete_person(b);
  // Segnala di iniziare il ciclo di vita
  segnala_start(new_1);
  segnala_start(new_2);
  // Ora birth_death puo' essere eseguito
  not = 0;
  rm_func();
}


void termina_persona(person* p) {
  add_func("termina_persona");
  int pid = p->pid;
  // manda il segnale
  kill(pid,SIGTERM);
  // esegue la wait
  waitpid(pid,NULL,0);
  // toglie la persona dalle strutture
  if (p->tipo == A) {
    // toglie A dalla memoria condivisa
    shm_pop(shmptr,pid);
    pop_person(a_people,pid);
  } else {
    pop_person(b_people,pid);
  }
  // svuota i messaggi pendenti
  empty_queue(p);
  // risetta il semaforo di start a 1 per il prossimo processo
  set_one(sem_start,p->sem,1);
  rm_func();
}


/* Svuota i messaggi contact di un processo terminato.
Se di tipo A, rifiuta tutte le richieste pendenti
Se di tipo B e c'e' un assenso di un tipo A, manda un messaggio di conferma negativo
*/
void empty_queue(person* p) {
  add_func("empty_queue");
  pid_t pid = p->pid;
  // messaggio da ricevere
  message r;
  // messaggio da inviare
  message s;
  s.data = 0;
  s.pid = pid;
  int status;
  if (p->tipo == A) { // A
    while ((status = msgrcv(msq_contact,&r,msgsize,pid,IPC_NOWAIT)) != -1 || (status == -1 && errno == EINTR)) {
      if (status == -1) continue;
      // setta l'mtype per rispondere al processo B
      s.mtype = r.pid;
      // rifiuta le richieste pendenti
      while (msgsnd(msq_contact,&s,msgsize,0) == -1 && errno == EINTR) continue;
    }
  } else { // B
    while (msgrcv(msq_contact,&r,msgsize,pid,IPC_NOWAIT) == -1 && errno == EINTR) continue;
    if (errno != ENOMSG) { // c'e' un messaggio di rifiuto/assenso
      if (r.data) { // assenso
        s.mtype = r.pid;
        // Manda messaggio di conferma negativo
        while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
      }
    }
  }
  rm_func();
}


void print_message(message* m) {
  add_func("print_message");
  printf("Pid: %d\tmtype: %lu\tpartner: %d\tdata: %d\n",m->pid,m->mtype,m->partner,m->data);
  rm_func();
}


/* Attende messaggi di accoppiamento */
void wait_for_messages() {
  add_func("wait_for_messages");
  while (1) {
    // Riceve il messaggio di un processo A
    while (msgrcv(msq_match,&a_mess,msgsize,getpid(),0) == -1 && errno == EINTR) continue;
    // Segnala a_mess birth_death che i processi sono in fase di matching
    a_matching = a_mess.pid;
    b_matching = a_mess.partner;
    //printf("matching: %d, %d\n",a_matching,b_matching);
    person* a = get_person(a_people,a_matching);
    person* b = get_person(b_people,b_matching);
    // Riceve il messaggio del processo B con IPC_NOWAIT
    while (msgrcv(msq_match,&b_mess,msgsize,a_mess.partner,IPC_NOWAIT) == -1 && errno == EINTR) continue;
    // Il messaggio e' stato ricevuto
    // controlla la validita' dei messaggi
    if (a_mess.pid != b_mess.partner || b_mess.pid != a_mess.partner) {
      debug(0);
    }
    int a_alive;
    int b_alive;
    // controlla che i processi siano ancora vivi
    if ((a_alive = kill(a_mess.pid,0)) == -1 || (b_alive = kill(b_mess.pid,0)) == -1) { // uno e' terminato
      // Uno dei due processi e' stato ucciso da birth_death
      // Restarta il processo in pausa.
      if (a_alive == -1) {
        segnala_start(b);
      } else {
        segnala_start(a);
      }
    } else {  // sono entrambi vivi
      // li accoppia
      accoppia(a,b);
    }
    // l'accoppiamento e' finito
    a_matching = 0;
    b_matching = 0;
  }
  rm_func();
}


/* Termina un processo e ne crea uno nuovo ogni BIRTH_DEATH secondi */
void birth_death(int sig) {
  add_func("birth_death");
  // Stampa le informazioni della simulazione
  print_info();
  // Calcola il tempo rimanente
  double _time = elapsed_time();
  double tempo_rimanente = SIM_TIME - _time;
  if (tempo_rimanente <= BIRTH_DEATH) {
    // Cambia l'handler di SIGALRM per terminare
    struct sigaction sa;
    sa.sa_handler = quit;
    sigfillset(&sa.sa_mask);
    sigaction(SIGALRM,&sa,NULL);
    // imposta il timer per finire nel momento corretto
    timer.it_value.tv_sec = (int)tempo_rimanente;
    timer.it_value.tv_usec = (tempo_rimanente-(int)tempo_rimanente)*1e6;
    // disabilita ulteriori timer, superfluo
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
  }
  if (a_people->length+b_people->length == 2 || not) {
    //printf("Birth Death not possible!\n");
    rm_func();
    return;
  }
  // sceglie una vittima
  person* victim = choose_victim();
  // la termina e pulisce le strutture
  termina_persona(victim);
  // Crea un nuovo individuo
  person* new = spawn_new_person(NULL,0,victim->sem);
  // Attende che il processo sia pronto
  wait_ready(new);
  // Segnala di iniziare
  segnala_start(new);
  rm_func();
}


/* Uccide tutti i processi figli e esegue le wait su di essi */
void kill_all() {
  add_func("kill_all");
  // ignora SIGTERM prima di inviare il segnale al gruppo di processi
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGTERM,&sa,NULL);
  kill(0,SIGTERM);
  errno = 0;
  while (wait(NULL) == 0 || errno == EINTR) continue;
  rm_func();
}


/* Manda il messaggio di start alla persona p */
void segnala_start(person* p) {
  add_func("segnala_start");
  add_start(p->sem,1);
  rm_func();
}

/* Attende che la persona p sia pronta ad iniziare il ciclo di vita */
void wait_ready(person* p) {
  add_func("wait_ready");
  wait_start(p->sem);
  rm_func();
}

void debug(int sig) {
  printf("\n%s\n",strsignal(sig));
  print_people(a_people);
  print_people(b_people);
  printf("Accoppiamento in corso: %d,%d\n",a_matching,b_matching);
  printf("\nMessaggio di A\n");
  print_message(&a_mess);
  printf("\nMessaggio di B\n");
  print_message(&b_mess);
  printf("\nStack:\n[");
  for (int i = 0;i < stack_length;i++) {
    printf("%s, ",stack[i]);
  }
  printf("]\n\nInfo: %d\n",debug_info);
  printf("\nStato memoria condivisa\n");
  print_all_shm(shmptr);
  printf("\n");
  print_sem_match();
  print_sem_start();
  people_for_each(a_people,debug_person);
  people_for_each(b_people,debug_person);
  sleep(1);
  quit(sig);
}

void debug_person(person* p) {
  kill(p->pid,SIGUSR2);
}

/* Gestisce i segnali con sigaction */
void set_signals() {
  add_func("set_signals");
  struct sigaction s_quit;
  struct sigaction s_birth_death;
  struct sigaction s_debug;
  struct sigaction s_ign;
  // assegna gli handler
  s_ign.sa_handler = SIG_IGN;
  s_debug.sa_handler = debug;
  s_quit.sa_handler = quit;
  s_birth_death.sa_handler = birth_death;
  // segnali non interrompibili
  sigfillset(&s_debug.sa_mask);
  sigfillset(&s_quit.sa_mask);
  // birth_death puo' essere interrotto dagli altri segnali
  sigemptyset(&s_birth_death.sa_mask);
  // handler per i segnali di terminazione
  sigaction(SIGINT,&s_debug,NULL);
  sigaction(SIGABRT,&s_debug,NULL);
  sigaction(SIGILL,&s_debug,NULL);
  sigaction(SIGSEGV,&s_debug,NULL);
  sigaction(SIGTERM,&s_quit,NULL);
  // Il birth-death avviene tramite il segnale SIGALRM
  sigaction(SIGALRM,&s_birth_death,NULL);
  sigaction(SIGUSR1,&s_ign,NULL);
  rm_func();
}

/* Inizia la simulazione */
void start() {
  add_func("start");
  // imposta il timer per birth_death
  setitimer(ITIMER_REAL,&timer,NULL);
  // Crea le prime persone
  for (short i = 0;i < INIT_PEOPLE;i++) {
    spawn_new_person(NULL,0,i);
  }
  // Attende che siano pronte
  people_for_each(a_people,wait_ready);
  people_for_each(b_people,wait_ready);
  // Manda il segnale di start
  set_all(sem_start,1);
  // Attende i messaggi di accoppiamento
  wait_for_messages();
  rm_func();
}

/* Handler dei segnali di terminazione.
Uccide i figli, cancella le strutture IPC e stampa le informazioni finali */
void quit(int sig) {
  print_final_info();
  kill_all();
  // cancella la memoria condivisa
  shm_destroy();
  // cancella le code di messaggi
  msgctl(msq_match,IPC_RMID,NULL);
  msgctl(msq_contact,IPC_RMID,NULL);
  // cancella i semafori
  sem_destroy();
  // chiude il file urandom
  fclose(urandom);
  // exit
  exit(EXIT_SUCCESS);
}

/* Inizializza le strutture IPC, le variabili di gestione,
gli handler dei segnali etc. */
void init() {
  // inizializza la memoria per lo stack
  for (int i = 0;i < 64;i++) {
    stack[i] = malloc(64);
  }
  add_func("init");
  // gestione segnali
  set_signals();
  // Definisce il tempo iniziale
  gettimeofday(&start_time,NULL);
  // Imposta il timer
  timer.it_value.tv_sec = BIRTH_DEATH;
  timer.it_interval.tv_sec = BIRTH_DEATH;
  // Apre il file urandom
  urandom = fopen("/dev/urandom", "r");
  // Crea le code di messaggi
  msq_init();
  msgsize = sizeof(message)-sizeof(long);
  // Crea i semafori
  sem_create();
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
  rm_func();
}

int main(int argc, char* argv[]) {
  INIT_PEOPLE = strtoul(argv[1],NULL,10);
  GENES = strtoul(argv[2],NULL,10);
  BIRTH_DEATH = strtoul(argv[3],NULL,10);
  SIM_TIME = strtoul(argv[4],NULL,10);
  init();
  start();
  //quit(0);
}
