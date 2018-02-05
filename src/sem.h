#ifndef SEM_H
#define SEM_H
#define SEM_START 111
#define SEM_MATCH 222

void sem_create();
void sem_destroy();
void sem_init();
void print_sem_match();
void print_sem_start();
void wait_match(short num);
void wait_start(short num);
void add_match(short num, short op);
void add_start(short num, short op);
void set_all(int id, short num);
void set_one(int id, short sem, short num);

#endif
