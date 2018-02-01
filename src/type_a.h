#ifndef TYPE_A_H
#define TYPE_A_H

void abbassa_target();
void push_contact(unsigned int id);
void accetta(pid_t pid);
void rifiuta(pid_t pid);
void ascolta();
void init();
void quit(int sig);
void debug(int sig);

#endif
