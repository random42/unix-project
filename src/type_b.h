#ifndef TYPE_B_H
#define TYPE_B_H

void shm_init();
void shm_detach();
int not_black_list(unsigned int id);
void accoppia(pid_t pid);
char contatta(pid_t pid);
void cerca_target();
void cerca();
void init();
void quit(int sig);

#endif
