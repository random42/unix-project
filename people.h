#define PEOPLE_H

type_t random_type();
person* create_person(char* name, int mcd, int type);
person* create_rand_person(int type);
people* create_people(int num);
void push_person(people* l, person* p);
void pop_person(people* l, pid_t pid);
people* init_people();
people* join(people* a, people* b);
void print_people(people* arr);
void print_person(person* p);
void people_for_each(people* arr, void (*f)(person*));
void delete_people(people* l);
