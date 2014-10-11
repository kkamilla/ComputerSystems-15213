
#include "cache.h"
#include "csapp.h"

unsigned int time_ctr = 0;
pthread_rwlock_t cache_lk;

void init_cache() {
    int status = pthread_rwlock_init(&cache_lk, 0);
    if (status != 0) {
	printf("Lock initialization error\n");
    }
}

cache_node *search(cache_list *cache, char *path) {
    int status = pthread_rwlock_rdlock(&cache_lk);
    /* Incrementing time counter */
    time_ctr++;
    cache_node *ptr = cache->head;/* pointer to traverse through cache */
    while(ptr != NULL) {
        /* Checking if the path is already present in the cache */
    	if(!strcmp(ptr->path, path)) {
    		/* Updating the time of node as it is read */
            ptr->time = time_ctr;
            return ptr;
        }
        ptr = ptr->next;
    }
    status = pthread_rwlock_unlock(&cache_lk);
    return NULL;
}


void add_node(cache_list *cache, char *path, char *content, unsigned int size) {
    int status = pthread_rwlock_wrlock(&cache_lk);

    /* Evict until a fit is found */
    while((cache->size + size) > MAX_CACHE_SIZE)
    	evict_node(cache);
    time_ctr++;
    cache_node *new_entry = Malloc(sizeof(cache_node));
    
    /* Updating the values in the new node */
    new_entry->content = Malloc(MAX_OBJECT_SIZE); // here we can malloc only the required size 
    new_entry->path = Malloc(MAXLINE);
    memcpy(new_entry->content, content, size);/* Copy byte by byte */
    new_entry->size = size;
    new_entry->time = time_ctr;
    strcpy(new_entry->path, path);
    /* Updating cache size */
    cache->size = cache->size + size;

    /* New node will be added as head of cache to make it faster */
    new_entry->next = cache->head;
    cache->head = new_entry;  
    status = pthread_rwlock_unlock(&cache_lk);
}

void evict_node(cache_list *cache) {
    int status = pthread_rwlock_wrlock(&cache_lk);
	
    /* Initialize least time as time of head node initially */
    cache_node *ptr = cache->head;
    unsigned int least_time = cache->head->time;
    while(ptr != NULL)
    {
        if(ptr->time < least_time)
            least_time = ptr->time;
        ptr = ptr->next;
    }

    /* Section to delete the appropriate node */
    ptr = cache->head;
    cache_node *temp;
    /* If node to be evicted is at head */
    if( cache->head->time == least_time)
    {
        temp = cache->head;
        cache->head = cache->head->next;
        cache->size =cache->size - (temp->size);
        free(temp->content);
        free(temp->path);
        free(temp);
    }

    /* If node to be evicted is not the head */
    while(ptr->next != NULL)
    {	
        if(ptr->next->time == least_time)
        {
            temp = ptr->next;
            ptr->next = ptr->next->next;
            cache->size = cache->size - (temp->size);
        	free(temp->content);
        	free(temp->path);
        	free(temp);
        	return;
        }

        ptr = ptr->next;
    }
    status = pthread_rwlock_unlock(&cache_lk);
}
