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
person* create_person(char* name, unsigned long mcd, int type) {
  int name_length = strlen(name)+1;
  // alloca la memoria per la nuova persona
  person* r = malloc(sizeof(person));
  r->nome = malloc(name_length+1);
  // copia il nome del genitore
  strcpy(r->nome,name);
  unsigned short n;
  fread(&n,sizeof(unsigned short),1,urandom);
  // inserisce una nuova lettera random nel nome
  r->nome[name_length-1] = (n % 26) + 65;
  r->nome[name_length] = '\0';
  unsigned long gen;
  fread(&gen,sizeof(unsigned long),1,urandom);
  // genera il genoma
  r->genoma = (gen % GENES+1) + mcd;
  // definisce il tipo
  r->tipo = (type == A || type == B) ? type : random_type();
  return r;
}


/* Genera una struttura person random
L'argomento type funziona come in create_person */
person* create_rand_person(int type) {
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
  printf("%d\t%s\t%lu\t%d\n",p->id,(p->tipo == 0 ? "A":"B"),p->genoma,p->pid);
}


/* Stampa le informazioni di una lista di persone */
void print_people(people* arr) {
  printf("ID\tTIPO\tGENOMA\tPID\n\n");
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
    n = n->next;
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


/* Libera la memoria della lista */
void delete_people(people* l) {
  node* n = l->first;
  node* next = n->next;
  for (int i = 0;i < l->length;i++) {
    delete_person(n->elem);
    free(n);
    n = next;
    next = n->next;
  }
  free(l);
}

/* Libera la memoria della persona */
void delete_person(person* p) {
  free(p->nome);
  free(p);
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


person* get_longest_name(people* arr) {
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
  return r;
}

person* get_greatest_genoma(people* arr) {
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
  return r;
}
