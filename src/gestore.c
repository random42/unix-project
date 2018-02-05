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

int shmid;
void* shmptr;

int msq_match;
int msq_start;
int msq_contact;
int msgsize;

char* stack[64];
int stack_length;
int debug_info;


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
  if ((msq_start = msgget(MSG_START,IPC_CREAT /* | IPC_EXCL */ | 0600)) == -1) {
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


/* Ritorna il processo con genoma minimo o NULL nel caso
speciale in cui gli individui esistenti siano 2 */
person* choose_victim() {
  add_func("choose_victim");
  if (a_people->length + b_people->length == 2) {
    return NULL;
  }
  node* n = a_people->first;
  person* p;
  // Un minimo iniziale maggiore di tutti i genomi
  unsigned long min = GENES*GENES;
  for (int i = 0;i < a_people->length;i++) {
    if (n->elem->genoma < min && n->elem->pid != a_matching) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  n = b_people->first;
  for (int i = 0;i < b_people->length;i++) {
    if (n->elem->genoma < min && n->elem->pid != b_matching) {
      p = n->elem;
      min = p->genoma;
    }
    n = n->next;
  }
  rm_func();
  return p;
}

/*
Immette un nuovo individuo nella popolazione.
Se nome e' diverso da NULL l'individuo e' generato da un genitore,
altrimenti avra' le caratteristiche iniziali. La funzione provvede
a non estinguere nessuno dei due tipi.
*/
person* spawn_new_person(char* nome, unsigned long mcd) {
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
  // scrivo genoma e id in una stringa da passare come argomento dell'execl
  char* genoma = malloc(64);
  sprintf(genoma,"%lu",p->genoma);
  char* id = malloc(64);
  sprintf(id,"%u",p->id);
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
        execl(a_path,"a",p->nome,genoma,people,NULL);
        printf("Execl not working\n");
      } else {
        execl(b_path,"b",p->nome,genoma,id,people,NULL);
        printf("Execl not working\n");
      }
    }
    default: { // padre
      p->pid = child;
    }
  }
  rm_func();
}


/* Riceve i pid dei processi accoppiati, pulisce le loro strutture e
crea gli individui figli */
void accoppia(int a, int b) {
  add_func("accoppia");
  // aumenta il numero di accoppiamenti totali
  matches++;
  // toglie A dalla memoria condivisa
  shm_pop(shmptr,a);
  // esegue le wait
  waitpid(a,NULL,0);
  waitpid(b,NULL,0);
  // prende le persone dalla lista
  person* old_a = get_person(a_people,a);
  person* old_b = get_person(b_people,b);
  //print_person(old_a);
  //print_person(old_b);
  // svuota i messaggi
  empty_queue(old_a);
  empty_queue(old_b);
  // in questo contesto serve solamente a prelevare il messaggio di terminazione
  end_match(old_a);
  end_match(old_b);
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
  rm_func();
}

/* Gestisce i match incompleti a causa di birth_death */
void end_match(person* p) {
  add_func("end_match");
  // riceve il messaggio con la fase di match e il partner
  message r;
  message s;
  while (msgrcv(msq_start,&r,msgsize,p->pid,0) == -1 && errno == EINTR) continue;
  int partner = r.partner;
  int match_phase = r.data;
  if (p->tipo == A) { // caso A
    s.data = 0;
    switch (match_phase) {
      case 1: { // deve ancora rispondere al contatto di B
        printf("A fase 1\n");
        s.data = 0;
        s.mtype = partner;
        while (msgsnd(msq_contact,&s,msgsize,0) == -1 && errno == EINTR) continue;
        break;
      }
      case 2: {  // Deve attendere la conferma di B e segnalare a B di continuare
        printf("A fase 2\n");
        // riceve il messaggio di conferma
        while (msgrcv(msq_match,&r,msgsize,p->pid,0) == -1 && errno == EINTR) continue;
        // preleva il messaggio che B ha mandato al gestore
        while (msgrcv(msq_match,&r,msgsize,partner,IPC_NOWAIT) == -1 && errno == EINTR) continue;
        // segnala a B di continuare
        kill(partner,SIGUSR1);
        break;
      }
      case 3: {  // Deve segnalare a B di continuare
        printf("A fase 3\n");
        // preleva il messaggio che B ha mandato al gestore
        while (msgrcv(msq_match,&r,msgsize,partner,IPC_NOWAIT) == -1 && errno == EINTR) continue;
        // segnala a B di continuare
        kill(partner,SIGUSR1);
        break;
      }
    }
  }
  else {  // caso B
    s.data = 0;
    s.mtype = partner;
    switch (match_phase) {
      case 1: { // ha contattato A
        printf("B case 1\n");
        // Riceve il messaggio di assenso/rifiuto
        while (msgrcv(msq_contact,&r,msgsize,p->pid,0) == -1 && errno == EINTR) continue;
        if (!r.data) { // A non ha accettato
          break;
        }
        // se ha accettato passa al caso 2
      }
      case 2: { // A ha accettato, non conferma il match
        printf("B case 2\n");
        while (msgsnd(msq_match,&s,msgsize,0) == -1 && errno == EINTR) continue;
        break;
      }
    }
  }
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
    // Riceve il messaggio del processo B con IPC_NOWAIT
    while (msgrcv(msq_match,&b_mess,msgsize,a_mess.partner,IPC_NOWAIT) == -1 && errno == EINTR) continue;
    // Il messaggio e' stato ricevuto
    //controlla la validita' dei messaggi
    if (a_mess.pid != b_mess.partner || b_mess.pid != a_mess.partner) {
      debug(0);
    }
    // controlla che i processi siano ancora vivi
    if (kill(a_mess.pid,0) + kill(b_mess.pid,0) < 0) { // uno e' terminato
      // Uno dei due processi e' stato ucciso da birth_death
      // prima della ricezione del messaggio di A
      // Restarta il processo in pausa.
      printf("Uno dei processi e' terminato\n");
      kill(a_mess.pid,SIGUSR1);
      kill(b_mess.pid,SIGUSR1);
      a_matching = 0;
      b_matching = 0;
    } else {  // sono entrambi vivi
      // segnala ai processi di terminare
      kill(b_mess.pid,SIGTERM);
      kill(a_mess.pid,SIGTERM);
      // li accoppia
      accoppia(a_mess.pid,a_mess.partner);
    }
  }
  rm_func();
}


/* Termina un processo e ne crea uno nuovo ogni BIRTH_DEATH secondi */
void birth_death(int sig) {
  add_func("birth_death");
  double tempo_rimanente = SIM_TIME - elapsed_time();
  if (tempo_rimanente <= BIRTH_DEATH) { // Cambia l'handler di SIGALRM per terminare
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
  if (a_people->length+b_people->length == 2) {
    printf("Birth Death not possible!\n");
    return;
  }
  // sceglie una vittima
  person* victim;
  while ((victim = choose_victim()) == NULL) continue;
  printf("Victim:\n");
  print_person(victim);
  pid_t pid = victim->pid;
  // manda il segnale di terminazione al processo
  if (kill(pid, SIGTERM) == -1) {
    raise(SIGTERM);
  }
  if (victim->tipo == A) {
    // rimuove la persona dalla memoria condivisa
    shm_pop(shmptr,pid);
    // rimuove la persona dalla lista di persone di quel tipo
    pop_person(a_people,pid);
  } else {
    pop_person(b_people,pid);
  }
  // esegue la wait
  waitpid(pid,NULL,0);
  // gestisce i messaggi e il matching
  end_match(victim);
  empty_queue(victim);
  // Crea un nuovo individuo
  person* new = spawn_new_person(NULL,0);
  // Attende che il processo sia pronto
  ready_receive(new);
  // Segnala di iniziare
  segnala_start(new);
  printf("New %d\n",new->pid);
  // Stampa le informazioni della simulazione
  print_info();
  rm_func();
}


/* Uccide tutti i processi figli e esegue le wait su di essi */
void kill_all() {
  add_func("kill_all");
  people_for_each(a_people,kill_person);
  people_for_each(a_people,kill_person);
  errno = 0;
  while (wait(NULL) == 0 || errno == EINTR) continue;
  rm_func();
}


/* Manda il segnale di terminazione alla persona p */
void kill_person(person* p) {
  add_func("kill_person");
  kill(p->pid,SIGTERM);
  rm_func();
}


/* Manda il messaggio di start alla persona p */
void segnala_start(person* p) {
  add_func("segnala_start");
  kill(p->pid,SIGUSR1);
  rm_func();
}


/* Riceve il messaggio di pronto della persona p */
void ready_receive(person* p) {
  add_func("ready_receive");
  message x;
  debug_info = p->pid;
  while (msgrcv(msq_start,&x,msgsize,p->pid,0) == -1 && errno == EINTR) continue;
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
  // assegna gli handler
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
  rm_func();
}

/* Inizia la simulazione */
void start() {
  add_func("start");
  // imposta il timer per birth_death
  setitimer(ITIMER_REAL,&timer,NULL);
  // Crea le prime persone
  for (int i = 0;i < INIT_PEOPLE;i++) {
    spawn_new_person(NULL,0);
  }
  // Attende che siano pronte
  people_for_each(a_people,ready_receive);
  people_for_each(b_people,ready_receive);
  // Inizia il ciclo di vita
  people_for_each(a_people,segnala_start);
  people_for_each(b_people,segnala_start);
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
  msgctl(msq_start,IPC_RMID,NULL);
  msgctl(msq_contact,IPC_RMID,NULL);
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
}
