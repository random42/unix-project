#ifndef SEM_H
#define SEM_H
#define SEM_START 111
#define SEM_MATCH 222

void sem_create();
void sem_destroy();
void sem_init();
void print_sem_match();
void print_sem_start();
void set_match(short num, short op);
void set_start(short num, short op);

#endif
