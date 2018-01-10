#include <stdio.h>
#include <stdlib.h>
#include <strin.h>
#ifndef HEADER_H
#include "header.h"
#endif

int compare_people(person* a, person* b) {
  return ((a->tipo == b->tipo) && (a->genoma == b->genoma) && (a->pid == b->pid) && (strcmp(a->nome,b->nome) == 0));
}

void person_push(people* l, person* p) {
  node* n = malloc(sizeof(node));
  if (l->length == 0) {
    l->first = n;
    l->last = n;
    n->prev = NULL;
  } else {
    l->last->next = n;
    n->prev = l->last;
  }
  n->elem = p;
  l->last = n;
  n->next = NULL;
  l->length++;
}

void person_pop(people* l, person* p) {
  node* n = (l->first);
  int i = 0;
  int found = 0;
  while (i < l->length && !found) {
    found = compare_people(n->elem,p);
    i++;
    n = n->next;
  }
  if (found) {
    node* next = n;
    node* prev = n->prev->prev;
    n = n->prev;
    next->prev = prev;
    prev->next = next;
  }
}

people* init_people() {
  people a;
  a.first = NULL;
  a.last = NULL;
  a.length = 0;
  return &a;
}
