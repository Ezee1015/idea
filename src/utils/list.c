#include <stdlib.h>

#include "list.h"

void list_append(List *list, void *element) {
  list_insert_at(list, element, list->count);
}

List_node *list_node_get(List list, unsigned int pos) {
  if (pos >= list.count) abort();

  if (pos == list.count-1) return list.last;

  List_iterator iterator = list_iterator_create(list);
  while (list_iterator_next(&iterator) && pos--);
  if (list_iterator_finished(iterator)) abort();
  return list_iterator_node(iterator);
}

void *list_get(List list, unsigned int pos) {
  return ((List_node *) list_node_get(list, pos))->pointer;
}

void list_insert_at(List *list, void *element, unsigned int pos) {
  if (!list) abort();
  if (!element) abort();

  List_node *node = malloc(sizeof(List_node));
  node->pointer = element;
  node->next = NULL;

  if (pos == 0) {
    node->next = list->head;
    list->head = node;
    if (list_is_empty(*list)) list->last = node;
  } else if (pos == list->count) {
    list->last->next = node;
    list->last = node;
  } else {
    List_node *prev_node = list_node_get(*list, pos-1);
    node->next = prev_node->next;
    prev_node->next = node;
  }

  list->count++;
}

unsigned int list_size(List list) {
  return list.count;
}

bool list_is_empty(List list) {
  return (!list.count);
}

void *list_remove(List *list, unsigned int pos) {
  if (!list) abort();
  if (pos >= list->count) abort();

  List_node *remove;

  if (pos == 0) {
    remove = list->head;
    list->head = remove->next;
  } else {
    List_node *prev_node = list_node_get(*list, pos-1);
    remove = prev_node->next;
    if (pos == list->count-1) list->last = prev_node;
    prev_node->next = remove->next;
  }

  list->count--;
  void *remove_data = remove->pointer;
  free(remove);
  return remove_data;
}

bool list_map_bool(List list, bool (*operation)(void *)) {
  if (!operation) abort();

  List_iterator iterator = list_iterator_create(list);
  while (list_iterator_next(&iterator)) {
    if (!operation(list_iterator_element(iterator))) return false;
  }
  return true;
}

void list_destroy(List *list, void (*free_element)(void *)) {
  List_node *aux = list->head;
  while (aux) {
    if (free_element) free_element(aux->pointer);
    List_node *remove = aux;
    aux = aux->next;
    free(remove);
  }

  list->head = list->last = NULL;
  list->count = 0;
}

bool list_save_to_bfile(List list, bool (*save_element_to_bfile)(FILE *, void *), FILE *file) {
  if (!file) abort();

  if (!fwrite(&list.count, sizeof(unsigned int), 1, file)) return false;

  List_iterator iterator = list_iterator_create(list);
  while (list_iterator_next(&iterator)) {
    void *element = list_iterator_element(iterator);
    if (!save_element_to_bfile(file, element)) return false;
  }

  return true;
}

bool list_load_from_bfile(List *list, void *(*read_element_from_bfile)(FILE *), FILE *file) {
  if (!file) abort();

  unsigned int elements;
  if (!fread(&elements, sizeof(unsigned int), 1, file)) return false;

  for (unsigned int i=0; i<elements; i++) {
    list_append(list, read_element_from_bfile(file));
  }

  return true;
}

unsigned int list_peek_element_count_from_bfile(FILE *file) {
  if (!file) abort();

  unsigned int elements;
  if (!fread(&elements, sizeof(unsigned int), 1, file)) return -1;
  if (fseek(file, -1 * sizeof(unsigned int), SEEK_CUR)) return -1;

  return elements;
}

List_iterator list_iterator_create(List list) {
  return (List_iterator){
    .current = NULL,
    .next = list.head,
    .index = -1
  };
}

bool list_iterator_finished(List_iterator iterator) {
  return ( !iterator.current );
}

bool list_iterator_has_next(List_iterator iterator) {
  return ( iterator.next );
}

bool list_iterator_next(List_iterator *iterator) {
  if (!iterator) abort();

  iterator->current = iterator->next;
  iterator->index++;
  if (list_iterator_finished(*iterator)) return false;

  iterator->next = iterator->next->next;
  return true;
}

unsigned int list_iterator_index(List_iterator iterator) {
  return iterator.index;
}

List_node *list_iterator_node(List_iterator iterator) {
  if (list_iterator_finished(iterator)) abort();
  return iterator.current;
}

void *list_iterator_element(List_iterator iterator) {
  return list_iterator_node(iterator)->pointer;
}
