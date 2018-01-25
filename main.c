#include <stdio.h> // scanf e printf
#include <string.h> // stringhe
#include <stdlib.h> // libreria standard
#include "lista.h"

int main() {
  lista a = crea_nodo("ciao");
  a = inserisci_in_testa(a,"mamma");
  a = inserisci_in_coda(a,"bella");
  a = inserisci_in_coda(a,"gesoo");
  a = inserisci_in_coda(a,"cristo");
  boolean b = contiene(a,"ciao");
  lista c = puntatore_a_stringa(a,"asd");
  a = rimuovi_in_testa(a);
}
