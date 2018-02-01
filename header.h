#define HEADER_H
#define BIRTH_DEATH 4
#define SIM_TIME 20
#define GENES 50
#define INIT_PEOPLE 20
#define MSG_MATCH 999
#define MSG_START 888
#define MSG_CONTACT 777
#define print_error() printf("%s\n",strerror(errno))
#define debug_func(x) strcpy(debug_func,x)

// Struttura dei messaggi
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
