#define TYPE_A_H

void find_divisors();
unsigned long mcd(unsigned long a, unsigned long b);
void abbassa_target();
void push_contact(unsigned int id);
void accetta(pid_t pid);
void rifiuta(pid_t pid);
void ascolta();
void set_signals();
void msq_init();
void ready();
void do_nothing(int sig);
void init();
void quit(int sig);
void debug(int sig);
