#define HEADER_H
#define BIRTH_DEATH 4
#define SIM_TIME 20
#define GENES 50
#define INIT_PEOPLE 40
#define MSG_MATCH 9999
#define MSG_START 9998
#define print_error() printf("%s\n",strerror(errno))


typedef struct {
    long mtype;
    /* pid del ricevente eccetto
    B -> Gestore: pid di B
    questo e' per ricevere sempre i messaggi della coppia insieme
    */
    unsigned int id;
    // id del processo B tra B -> A
    char data;
    /* Questo campo ha diversi significati a seconda
    di chi manda e chi riceve il messaggio:
    B -> A: nullo
    A -> B: 0 per rifiuto, 1 per consenso
    A||B -> Gestore: 0 se il mittente e' di tipo B, 1 se di tipo A
    */
    unsigned long genoma;
    // usato solo tra processi A e B per comunicare il proprio genoma
    pid_t pid;
    // PID del mittente
    pid_t partner;
    // PID del parter, usato quando due processi si accoppiano e comunicano al gestore
} message;
