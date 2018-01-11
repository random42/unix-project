#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef HEADER_H
#include "header.h"
#endif
#ifndef PEOPLE_H
#include "people.h"
#endif

extern FILE* urandom;

/* Genera un tipo (A,B) random */
type_t random_type() {
  short num;
  fread(&num, sizeof(short), 1, urandom);
  return abs(num%2);
}


/* Crea una persona tramite il nome del genitore e l'MCD.
L'argomento type puo' forzare la scelta del tipo
type == 0 -> A
type == 1 -> B
default -> random */
person* create_person(char* name, int mcd, int type) {
  person* r = malloc(sizeof(person));
  if (strlen(name) > MAX_NAME-2) {
    printf("Reached maximum name length.\nExiting...\n");
    exit(EXIT_FAILURE);
  }
  strcpy(r->nome,name);
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  r->nome[strlen(name)] = (n % 26) + 65;
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  r->genoma = (gen % GENES) + mcd;
  r->tipo = (type == A || type == B) ? type : random_type();
  return r;
}


/* Genera una struttura person random
L'argomento type funziona come in create_person */
person* create_rand_person(int type) {
  person* r = malloc(sizeof(person));
  r->tipo = (type == A || type == B) ? type : random_type();
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  r->genoma = (gen % GENES)+2;
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  *(r->nome) = (n % 26) + 65;
  r->pid = 0;
  return r;
}


/* Genera num persone random */
people* create_people(int num) {
  people* p = init_people();
  int i = 0;
  while (i < num) {
    push_person(p,create_rand_person(-1));
    i++;
  }
  return p;
}


/* Esegue una funzione di tipo void su ogni elemento della lista */
void people_for_each(people* arr, void (*f)(person*)) {
  node* n = arr->first;
  for (int i = 0; i < arr->length;i++) {
    (*f)(n->elem);
    n = n->next;
  }
}


/* Stampa le informazioni di una persona */
void print_person(person* p) {
  printf("%s\t%s\t%lu\t%d\n",(p->tipo == 0 ? "A":"B"),p->nome,p->genoma,p->pid);
}


/* Stampa le informazioni di una lista di persone */
void print_people(people* arr) {
  printf("TIPO\tNOME\tGENOMA\tPID\n\n");
  people_for_each(arr,print_person);
  printf("\n");
}


/* Cerca una persona tramite pid, ritorna NULL se non esiste */
person* get_person(people* l, pid_t pid) {
  int i = 0;
  node* n = l->first;
  int found = 0;
  person* p = NULL;
  while (i < l->length && !found) {
    if (n->elem->pid == pid) {
      p = n->elem;
      found = 1;
    }
    i++;
  }
  return p;
}


/* Inserisce una persona all'inizio della lista */
void push_person(people* l, person* p) {
  node* n = malloc(sizeof(node));
  n->next = l->first;
  l->first = n;
  n->elem = p;
  l->length++;
}


/* Toglie la persona con il pid specificato dalla lista */
void pop_person(people* l, pid_t pid) {
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
      free(n->elem);
      free(n);
      l->length--;
    }
    else {
      i++;
      prev = n;
      n = n->next;
    }
  }
}


/* Libera la memoria di tutta la lista */
void delete_people(people* l) {
  node* n = l->first;
  node* next = n->next;
  for (int i = 0;i < l->length;i++) {
    free(n->elem);
    free(n);
    n = next;
    next = n->next;
  }
  free(l);
}


/* Alloca la memoria per la lista */
people* init_people() {
  people* a = malloc(sizeof(people));
  a->first = NULL;
  a->length = 0;
  return a;
}


/* Unisce due liste */
people* join(people* a, people* b) {
  people* r = init_people();
  r->first = a->first;
  node* n = r->first;
  for (int i = 0;i < a->length-1;i++) {
    n = n->next;
  }
  n->next = b->first;
  r->length = a->length+b->length;
  return r;
}
