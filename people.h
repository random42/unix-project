#define PEOPLE_H

typedef enum {A,B} type_t;

typedef struct {
  unsigned int id;
  type_t tipo;
  unsigned long genoma;
  char* nome;
  pid_t pid;
} person;

typedef struct nodo {
  person* elem;
  struct nodo* next;
} node;


typedef struct {
  node* first;
  int length;
} people;


type_t random_type();
person* create_person(char* name, unsigned long mcd, int type);
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
void delete_person(person* p);
person* get_person(people* l, pid_t pid);
person* get_longest_name(people* arr);
person* get_greatest_genoma(people* arr);
