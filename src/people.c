#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "header.h"
#include "people.h"

extern person* best_genoma;
extern person* longest_name;
extern unsigned long GENES;
extern char* stack[];
extern int stack_length;
extern int debug_info;
extern FILE* urandom;

/* Genera un tipo (A,B) random */
type_t random_type() {
  add_func("random_type");
  unsigned short num;
  fread(&num, sizeof(short), 1, urandom);
  rm_func();
  return num%2;
}


/* Crea una persona tramite il nome del genitore e l'MCD.
L'argomento type puo' forzare la scelta del tipo
type == 0 -> A
type == 1 -> B
default -> random */
person* create_person(char* name, unsigned long mcd, int type) {
  add_func("create_person");
  debug_info = 1;
  int name_length = strlen(name)+1;
  // alloca la memoria per la nuova persona
  debug_info = 2;
  person* r = malloc(sizeof(person));
  debug_info = 3;
  r->nome = malloc(name_length+1);
  debug_info = 4;
  // copia il nome del genitore
  strcpy(r->nome,name);
  debug_info = 5;
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  debug_info = 6;
  // inserisce una nuova lettera random nel nome
  r->nome[name_length-1] = (n % 26) + 65;
  r->nome[name_length] = '\0';
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  debug_info = 7;
  // genera il genoma
  r->genoma = (gen % GENES+1) + mcd;
  // definisce il tipo
  r->tipo = (type == A || type == B) ? type : random_type();
  rm_func();
  return r;
}


/* Genera una struttura person random
L'argomento type funziona come in create_person */
person* create_rand_person(int type) {
  add_func("create_rand_person");
  person* r = malloc(sizeof(person));
  r->nome = malloc(2);
  r->tipo = (type == A || type == B) ? type : random_type();
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  r->genoma = (gen % GENES+1)+2;
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  *(r->nome) = (n % 26) + 65;
  r->nome[1] = '\0';
  r->pid = 0;
  rm_func();
  return r;
}


/* Genera num persone random */
people* create_people(int num) {
  add_func("create_people");
  people* p = init_people();
  int i = 0;
  while (i < num) {
    push_person(p,create_rand_person(-1));
    i++;
  }
  rm_func();
  return p;
}


/* Esegue una funzione di tipo void su ogni elemento della lista */
void people_for_each(people* arr, void (*f)(person*)) {
  add_func("people_for_each");
  node* n = arr->first;
  for (int i = 0; i < arr->length;i++) {
    (*f)(n->elem);
    n = n->next;
  }
  rm_func();
}


/* Stampa le informazioni di una persona */
void print_person(person* p) {
  add_func("print_person");
  printf("%d\t%s\t%lu\t%d\n",p->id,(p->tipo == 0 ? "A":"B"),p->genoma,p->pid);
  rm_func();
}


/* Stampa le informazioni di una lista di persone */
void print_people(people* arr) {
  add_func("print_person");
  printf("ID\tTIPO\tGENOMA\tPID\n\n");
  people_for_each(arr,print_person);
  printf("\n");
  rm_func();
}


/* Cerca una persona tramite pid, ritorna NULL se non esiste */
person* get_person(people* l, pid_t pid) {
  add_func("get_person");
  debug_info = pid;
  int i = 0;
  node* n = l->first;
  int found = 0;
  person* p = NULL;
  while (i < l->length && !found) {
    if (n->elem->pid == pid) {
      p = n->elem;
      found = 1;
    }
    n = n->next;
    i++;
  }
  rm_func();
  return p;
}


/* Inserisce una persona all'inizio della lista */
void push_person(people* l, person* p) {
  add_func("push_person");
  node* n = malloc(sizeof(node));
  n->next = l->first;
  l->first = n;
  n->elem = p;
  l->length++;
  rm_func();
}


/* Toglie la persona con il pid specificato dalla lista */
void pop_person(people* l, pid_t pid) {
  add_func("pop_person");
  node* n = l->first;
  node* prev = NULL;
  int i = 0;
  int found = 0;
  while (i < l->length && !found) {
    found = n->elem->pid == pid;
    if (found) {
      if (i == 0) {
        l->first = n->next;
      } else {
        prev->next = n->next;
      }
      free(n);
      l->length--;
    }
    else {
      i++;
      prev = n;
      n = n->next;
    }
  }
  rm_func();
}


/* Libera la memoria della lista */
void delete_people(people* l) {
  add_func("delete_people");
  node* n = l->first;
  node* next = n->next;
  for (int i = 0;i < l->length;i++) {
    delete_person(n->elem);
    free(n);
    n = next;
    next = n->next;
  }
  free(l);
  rm_func();
}

/* Libera la memoria della persona */
void delete_person(person* p) {
  add_func("delete_person");
  if (p != best_genoma && p != longest_name) {
    free(p->nome);
    free(p);
  }
  rm_func();
}


/* Alloca la memoria per la lista */
people* init_people() {
  add_func("init_people");
  people* a = malloc(sizeof(people));
  a->first = NULL;
  a->length = 0;
  rm_func();
  return a;
}


person* get_longest_name(people* arr) {
  add_func("get_longest_name");
  node* n = arr->first;
  person* r = n->elem;
  char* nome = r->nome;
  for (int i = 0; i < arr->length;i++) {
    if (strlen(n->elem->nome) > strlen(nome)) {
      nome = n->elem->nome;
      r = n->elem;
    }
    n = n->next;
  }
  rm_func();
  return r;
}

person* get_greatest_genoma(people* arr) {
  add_func("get_greatest_genoma");
  node* n = arr->first;
  person* r = n->elem;
  unsigned long gen = r->genoma;
  for (int i = 0; i < arr->length;i++) {
    if (n->elem->genoma > gen) {
      gen = n->elem->genoma;
      r = n->elem;
    }
    n = n->next;
  }
  rm_func();
  return r;
}
