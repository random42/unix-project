# Unix Project
University of Turin, Italy
Computer Science Department
Authors: Roberto Sero, Gianmarco Sciortino

## Come eseguire

Clonare il progetto:

`$ git clone https://github.com/robertosero/unix-project`

Compilare:

`$ cd unix-project`

`$ make all`

Eseguire /bin/gestore:

`$ ./bin/gestore`

***Importante!*** Eseguire il gestore sempre dalla root directory, non navigare in bin!


## Struttura e scelte progettuali

### Persone

File: *people.c*

Un individuo è descritto nella struttura person, definita come:

```c
typedef struct {
  unsigned int id;
  type_t tipo;
  unsigned long genoma;
  char* nome;
  pid_t pid;
} person;
```

in cui il tipo è definito:
```c
typedef enum {A,B} type_t;
```

L'id e' probabilmente superfluo in quanto i pid non vengono riutilizzati (almeno non su OS X), tuttavia definisce l'unicita' della persona.

La struttura *people* definisce uno stack di persone.

```c
typedef struct nodo {
  person* elem;
  struct nodo* next;
} node;

typedef struct {
  node* first;
  int length;
} people;
```

### Code di messaggi
Un messaggio e' definito nel tipo message:
```c
typedef struct {
    long mtype;
    /* pid del ricevente eccetto:
    B -> Gestore (accoppiamento): pid di B
    A||B -> Gestore (start): pid del mittente */
    unsigned int id;
    // B -> A: id di B
    char data;
    // A -> B: 0 per rifiuto, 1 per consenso
    unsigned long genoma;
    // B -> A: genoma di B
    pid_t pid;
    // pid del mittente
    pid_t partner;
    /* pid del partner, usato quando due processi si accoppiano
    e comunicano al gestore */
} message;
```

Il programma usa tre code di messaggi.

*msq_contact*: Messaggi di richiesta e rifiuto/assenso tra processi A e B

*msq_match*: Messaggi tra i processi A, B e Gestore durante l'accoppiamento

*msq_start*: Messaggi inviati dai processi A e B al Gestore per comunicare che sono pronti a iniziare il loro ciclo di vita

### Memoria condivisa

File: *shm.c*

Viene creata una zona di memoria condivisa tra il Gestore e i processi B contenente la lista di processi A attivi.

Viene definita la struttura a_person:
```c
typedef struct {
  unsigned int id;
  unsigned long genoma;
  pid_t pid;
  char valid; // byte di validita' del processo
} a_person;
```
Lo spazio di memoria ha una grandezza pari a:
```c
sizeof(int) + INIT_PEOPLE * sizeof(a_person)
```
I primi 4 byte sono un intero che descrive la lunghezza dell'array di persone A. Ogni elemento dell'array ha un campo *valid* che vale 1 se il processo e' attivo, 0 se e' terminato. Quando si aggiunge una nuova persona la si sostituisce al primo elemento non valido, altrimenti si aggiunge dopo l'ultimo elemento e si aumenta l'intero contenente la lunghezza.

### Segnali

Per ogni tipo di processo vengono impostati gli handler per alcuni segnali.

La funzione di **debug** stampa una stringa *add_func* contenente l'ultima funzione chiamata, un intero *debug_info* assegnabile a scelta e informazioni aggiuntive sullo stato del processo. Essa **termina** sempre il processo (o il programma, nel caso del gestore).

|| Gestore | A & B|
|---|---|---|
|SIGTERM|Termina il programma|Termina il processo|
|SIGINT|Debug|Ignorato, riceve il segnale di debug dal gestore|
|SIGUSR1|NULL|Handler vuoto, per continuare l'esecuzione dopo *pause()*|
|SIGUSR2|NULL|Debug, inviato dal gestore|
|SIGALRM|Funzione birth_death|Debug, impostato a inizio esecuzione con alarm(5) nel caso non si accoppiasse|
|SIGSEGV|Debug|Debug|

### Gestore

### Processi A

### Processi B
