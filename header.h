#define HEADER_H
#define BIRTH_DEATH 10
#define SIM_TIME 50
#define GENES 30
#define INIT_PEOPLE 20
#define MSG_KEY 9999
#define MAX_A 2048
#define MAX_NAME 256
#define SHM_LENGTH 4096
#define SHM_KEY 1234

typedef enum {A,B} type_t;

typedef struct {
  type_t tipo;
  unsigned long genoma;
  char nome[MAX_NAME];
  pid_t pid;
} person;


typedef struct {
  unsigned long genoma;
  pid_t pid;
  char valid;
} a_person;


typedef struct {
    long mtype; // pid del processo ricevente
    char data;
    /* Questo campo ha diversi significati a seconda
    di chi manda e chi riceve il messaggio:
    B -> A: nullo
    A -> B: 0 per rifiuto, 1 per consenso
    A||B -> Gestore: 0 se il mittente e' di tipo B, 1 se di tipo A
    */
    unsigned long genoma; // usato solo tra processi A e B per comunicare il proprio genoma
    pid_t pid;
    /* Anche qua, se il messaggio avviene tra processi A e B
    e' il genoma del mittente,
    se avviene da A||B al Gestore e' il genoma del processo accoppiato */
} message;


typedef struct nodo {
  person* elem;
  struct nodo* next;
} node;


typedef struct {
  node* first;
  int length;
} people;
