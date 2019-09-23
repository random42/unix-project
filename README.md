# unix-tinder

**University of Turin, Italy**

**Computer Science Department**

**Authors**: Roberto Sero, Gianmarco Sciortino

## How to run

Clone project:

`git clone https://github.com/robertosero/unix-project`

Compile:

`cd unix-project`

`make`

Execute (always from project root):

`./bin/gestore <init_people> <genes> <birth_death> <sim_time>`


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
  short sem; // numero del semaforo associato
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
    */
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

Il programma usa due code di messaggi.

*msq_contact*: Messaggi di richiesta e rifiuto/assenso tra processi A e B

*msq_match*: Messaggi tra i processi A, B e Gestore durante l'accoppiamento

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

### Semafori

File: *sem.c*

Vi sono due semafori con *init_people* elementi ciascuno, ogni elemento associato a un processo A o B attivo:

*sem_start*: Per il segnale di pronto (A|B), start (dal Gestore) o di continuazione nel caso in cui uno dei due processi accoppiati termina.

*sem_match*: Vale 1 quando il processo e' in fase di accoppiamento. 0 in fase di ricerca/ascolto o a fine accoppiamento.

### Segnali

Funzione *set_signal()* in *gestore.c* e *child.c*

Per ogni tipo di processo vengono impostati gli handler per alcuni segnali.

La funzione di **debug** stampa lo stack di funzioni, un intero *debug_info* assegnabile a scelta e informazioni aggiuntive sullo stato del processo. Essa **termina** sempre il processo (o il programma, nel caso del gestore).

|| Gestore | A & B|
|---|---|---|
|SIGTERM|Termina il programma|Termina il processo|
|SIGINT|Debug|Ignorato, riceve il segnale di debug dal gestore|
|SIGUSR2|NULL|Debug, inviato dal gestore|
|SIGALRM|Funzione birth_death|Handler vuoto, usato nei processi B (*segue*)|
|SIGSEGV/SIGILL/SIGABRT|Debug|Debug|

### Gestore

#### start()
Il Gestore, creato i primi individui, rimane in attesa di messaggi nella funzione *wait_for_messages*.

#### wait_for_messages()
A ogni ciclo riceve il messaggio del processo A (con *mtype* uguale al proprio pid), seguito dal messaggio di B (con *mtype* uguale al pid di B) tramite IPC_NOWAIT.
Imposta le variabili *matching* per evitare che *birth_death* uccida uno dei processi durante l'accoppiamento. Controlla che i processi siano vivi e prosegue con *accoppia()*. Se uno dei processi e' stato ucciso, segnala all'altro di continuare.

#### accoppia()
Termina le persone e ne pulisce le strutture. Durante la creazione dei nuovi individui entra in una sezione critica e assegna 1 a *not*, che segnala a *birth_death()* di non poter essere eseguita in quanto puo' portare a una situazione di stallo.

#### birth_death()
Stampa le informazioni della simulazione. Se gli individui totali sono maggiori di 2 e *not* vale 0,
sceglie una persona random non in fase di accoppiamento tramite *choose_victim()*. La termina e crea un nuovo individuo.

Se mancano meno di *birth_death* secondi al raggiungimento di *sim_time*, imposta l'allarme successivo al tempo esatto e cambia l'handler in *quit()*.

### A&B

File: *child.c* (funzioni comuni), *type_a.c*, *type_b.c*

#### ready()
Dopo aver inizializzato le proprie strutture, i processi figli segnalano di essere pronti decrementando il semaforo *sem_start*. Attende lo start con un altro decremento, possibile quando il gestore incrementa il semaforo (dando appunto lo start).

#### cerca() / ascolta()
Per massimizzare il genoma dei nuovi processi, sia gli individui A che gli individui B contattano e accettano tramite *target*. La variabile assume il valore dei propri divisori in ordine decrescente.

A ogni ciclo di *cerca()* il processo B scorre i processi A nella memoria condivisa e contatta quelli per cui l'mcd tra i genomi sia >= al target scelto. A ogni rifiuto aggiunge l'id del processo A a una *black_list*, in modo da non contattarlo nuovamente. Dopo ogni ciclo il target si abbassa, fino al raggiungimento di 1. Il ciclo successivo il target ritorna al massimo divisore (il genoma) e la black list viene cancellata.

A ogni ciclo di *ascolta()* il processo A riceve un messaggio di contatto e decide di accettarlo in base al proprio target. Dopo *init_people* rifiuti, il processo A abbassa il target.

#### accoppia() / accetta()
Quando un processo A accetta la richiesta di un processo B manda un messaggio su *msq_contact* con *mtype* uguale al pid di B e *data* uguale a 1. I processi entrano in fase di accoppiamento ponendo a 1 il proprio semaforo in *sem_match*. Il processo B invia il messaggio al Gestore e un messaggio di conferma ad A, cosi' che quando A invia il proprio messaggio al Gestore, egli possa sicuramente ricevere il messaggio di B con IPC_NOWAIT.

#### fine_match()

Entrambi i processi pongono a 0 il proprio semaforo di match, e cercano di decrementare il semaforo di start. Nel caso in cui l'accoppiamento vada a buon fine ricevono il segnale SIGTERM dal Gestore e terminano. Se invece uno dei processi viene ucciso da *birth_death()* il Gestore incrementa il semaforo di start del processo ancora vivo permettendogli di continuare la ricerca/l'ascolto.
