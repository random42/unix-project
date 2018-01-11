# unix-project

Progetto_Unix - Dipartimento_di_Informatica - Torino - Creato da : Roberto Sero e Gianmarco Sciortino 

# Relazione:

     Scelta progettuale :

           INDIVIDUI :
              Un individuo è descritto nella struttura person, definita con:
              
              struct {
              unsigned int id;
              type_t tipo;
              unsigned long genoma;
              char &nome;
              pid_t pid;
            }
            
             in cui il tipo è : typedef enum {A,B} type_t;
             
             L' ID e il PID di ogni individuo viene inizializzato nel momento in cui viene inizializzato un processo effettivo.
             Il PID ha funzione di identificatore per ogni processo ma è stato necessario aggiungere un ID univoco per ogni processo 
             per riuscire ad implementare una "lista nera" che fosse capace di tenere memoria dei processi già contattati.
             Di suguito il codice per la lista:
             
             
             
             
             
             Nel momento in cui un processo di tipo B trova un processo A che accetta la sua richiesta la lista viene cancellata.
             Un individuo B contatterà un individuo di tipo A, a questo punto verranno fatte delle valutazioni basate sull' M.C.D. e se 
             l' M.C.D. del tipo B soddisferà i requisiti imposti dal tipo A, la richiesta verrà accettata, altrimenti il tipo B abbasserà 
             il valore dell'M.C.D. che sta cercando ed effettuerà altre richieste, questo fino all'accettazione della richiesta del tipo B
             da parte del tipo A.
             Se il tipo A non troverà nessun individuo congruo al proprio M.C.D abbasserà il valore del proprio target.
             Se il tipo B non verrà accettato entro un numero massimo di richieste effettuabili, si auto-cancellerà. 
             Il codice addetto alla creazione/manipolazione di una lista di individui è il file : people.c
          
          
          GESTORE :
              Il gestore dei processi (contenuto nel file gestore.c) è quel modulo che ha il compito di gestire una o più simulazioni di 
              attività dei processi.
              Oltre alla generazione degli individui sia di tipo A che di tipo B, il gestore si occupa anche di gestire la memoria
              condivisa, allocando e disallocando spazio a seconda dei casi.
          
          PROCESSI A : 
          
          PROCESSI B :
          
          MEMORIA CONDIVISA :
              
