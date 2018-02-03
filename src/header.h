#ifndef HEADER_H
#define HEADER_H
#define MSG_MATCH 999
#define MSG_START 888
#define MSG_CONTACT 777
#define print_error() printf("%s\n",strerror(errno))
#define add_func(x) strcpy(stack[stack_length++],x)
#define rm_func() stack_length--

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

#endif
