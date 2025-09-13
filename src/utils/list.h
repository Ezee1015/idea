#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdbool.h>

struct List_node {
  void *pointer;
  struct List_node *next;
};
typedef struct List_node List_node;

typedef struct {
  List_node *current;
  List_node *next;
  unsigned int index;
} List_iterator;

typedef struct {
  List_node *head;
  List_node *last;
  unsigned int count;
} List;
#define list_new() (List) { 0 };

void list_append(List *list, void *element);

void list_insert_at(List *list, void *element, unsigned int pos);

void *list_remove(List *list, unsigned int pos);

List_node *list_node_get(List list, unsigned int pos);

void *list_get(List list, unsigned int pos);

unsigned int list_size(List list);

bool list_is_empty(List list);

// If free_element is NULL, it will not free the element, just the list
void list_destroy(List *list, void (*free_element)(void *));

bool list_map_bool(List list, bool (*operation)(void *));

bool list_save_to_bfile(List list, bool (*save_element_to_bfile)(FILE *, void *), FILE *file);

bool list_load_from_bfile(List *list, void *(*read_element_from_bfile)(FILE *), FILE *file);

unsigned int list_peek_element_count_from_bfile(FILE *file);

List_iterator list_iterator_create(List list);

bool list_iterator_finished(List_iterator iterator);

bool list_iterator_has_next(List_iterator iterator);

bool list_iterator_next(List_iterator *iterator);

unsigned int list_iterator_index(List_iterator iterator);

List_node *list_iterator_node(List_iterator iterator);

void *list_iterator_element(List_iterator iterator);

#endif
