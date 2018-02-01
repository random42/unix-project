#ifndef TYPE_B_H
#define TYPE_B_H

void shm_init();
void shm_detach();
unsigned long mcd(unsigned long a, unsigned long b);
int not_black_list(unsigned int id);
void accoppia(pid_t pid);
char contatta(pid_t pid);
void cerca_target();
void cerca();
void find_divisori();
void set_signals();
void msq_init();
void ready();
void init();
void do_nothing(int sig);
void quit(int sig);
void debug(int sig);

#endif
