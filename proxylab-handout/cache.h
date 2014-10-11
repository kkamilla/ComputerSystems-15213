
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_node {
  unsigned int size;
  unsigned int time;
  char *path;
  char *content;
  struct cache_node *next;
} cache_node;

typedef struct cache_list{
  cache_node *head;
  unsigned int size;
} cache_list;

void init_cache();
void add_node(cache_list *cache, char *path, char *content, unsigned int size);
void evict_node(cache_list *cache);
cache_node *search(cache_list *cache, char *path);

