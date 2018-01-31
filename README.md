# Unix Project

Computer Science Department
University of Turin, Italy
Authors: Roberto Sero, Gianmarco Sciortino

## Scelte progettuali

### Persone
Un individuo è descritto nella struttura person, definita come:

```c
struct {
unsigned int id;
type_t tipo;
unsigned long genoma;
char* nome;
pid_t pid;
}```

in cui il tipo è definito:
```c
typedef enum {A,B} type_t;```

L'ID e il PID di ogni individuo viene inizializzato nel momento in cui viene inizializzato un processo effettivo.
Il PID ha funzione di identificatore per ogni processo ma è stato necessario aggiungere un ID univoco per ogni processo
per riuscire ad implementare una lista che fosse capace di tenere in memoria i processi già contattati.

Nel momento in cui un processo di tipo B trova un processo A che accetta la sua richiesta la lista viene cancellata.
Un individuo B contatterà un individuo di tipo A, a questo punto verranno fatte delle valutazioni basate sull' M.C.D. e se
l' M.C.D. del tipo B soddisferà i requisiti imposti dal tipo A, la richiesta verrà accettata, altrimenti il tipo B abbasserà
il valore dell'M.C.D. che sta cercando ed effettuerà altre richieste, questo fino all'accettazione della richiesta del tipo B
da parte del tipo A.
Se il tipo A non troverà nessun individuo congruo al proprio M.C.D abbasserà il valore del proprio target.
Se il tipo B non verrà accettato entro un numero massimo di richieste effettuabili, si auto-cancellerà.
Il codice addetto alla creazione/manipolazione di una lista di individui è il file : people.c


### Gestore
Il gestore dei processi (contenuto nel file gestore.c) è quel modulo che ha il compito di gestire una o più simulazioni di
attività dei processi.
Oltre alla generazione degli individui sia di tipo A che di tipo B, il gestore si occupa anche di gestire la memoria
condivisa, allocando e disallocando spazio a seconda dei casi.

### Processi A :

### Processi B :

MEMORIA CONDIVISA :
