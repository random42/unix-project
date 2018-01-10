#define HEADER_H
#define BIRTH_DEATH 10
#define SIM_TIME 50
#define GENES 30
#define INIT_PEOPLE 20
#define SHM_KEY 1234
#define MSG_KEY 9999
#define MAX_A 2048

// Definisco un tipo per il genere del processo

typedef enum {A,B} type_t;


// E una struttura per il processo

typedef struct {
  type_t tipo;
  unsigned long genoma;
  char* nome;
  pid_t pid;
} person;


typedef struct {
    long mtype;
    char mtext[1];
    unsigned long genoma;
    pid_t pid;
} message;

typedef struct nodo {
  person* elem;
  struct nodo* next;
  struct nodo* prev;
} node;

typedef struct {
  node* first;
  node* last;
  int length;
} people;
