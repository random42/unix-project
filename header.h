#define HEADER_H
#define BIRTH_DEATH 4
#define SIM_TIME 20
#define GENES 50
#define INIT_PEOPLE 3
#define MSG_MATCH 999
#define MSG_START 888
#define MSG_CONTACT 777
#define print_error() printf("%s\n",strerror(errno))
#define debug_func(x) strcpy(debug_func,x)

// Struttura dei messaggi
typedef struct {
    long mtype;
    /* pid del ricevente eccetto
    B -> Gestore: pid di B
    questo e' per ricevere sempre i messaggi della coppia insieme
    */
    unsigned int id;
    // id del processo B tra B -> A
    char data;
    /*
    A -> B: 0 per rifiuto, 1 per consenso
    */
    unsigned long genoma;
    // usato solo tra processi A e B per comunicare il proprio genoma
    pid_t pid;
    // PID del mittente
    pid_t partner;
    // PID del parter, usato quando due processi si accoppiano e comunicano al gestore
} message;
