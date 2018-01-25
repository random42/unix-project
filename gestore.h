#define GESTORE_H

#ifndef PEOPLE_H
#include "people.h"
#endif
#ifndef HEADER_H
#include "header.h"
#endif
#ifndef SHM_H
#include "shm.h"
#endif

unsigned long mcd(unsigned long a, unsigned long b);
void shm_init();
void shm_destroy();
void print_time();
void print_info(int sig);
person* choose_victim();
person* spawn_new_person(char* nome, unsigned long mcd);
void gen_child(person* p);
void accoppia(int a, int b);
void empty_queue(pid_t pid, int type);
void wait_for_messages();
void birth_death(int sig);
void kill_all();
void kill_person(person* p);
void segnala_start(person* p);
void set_signals();
void ready_receive(person* p);
void start();
void quit();
void init();
