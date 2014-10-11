/*
 * cache.c
 *
 * Name:- Vishal Shahane
 * Andrew ID:- vshahane
 * Email ID:- vshahane@andrew.cmu.edu
 * 
 * Caching highlights:
 * 1. Singly linked list is used to implement the cache. Eviction is
 *    done using LRU technique.
 * 2. Read write locks (with semaphores) are used to control concurrent 
 *    access to cache
 * 
 */
 
#include "cache.h"
#include "csapp.h"

/* Define DEBUG to enable debugging output */
//#define DEBUG
#ifdef DEBUG
# define debug_printf(...) printf(__VA_ARGS__)
#else
# define debug_printf(...)
#endif

/* Defining Global variables */
unsigned int time_counter = 0;

/* Read write lock */
pthread_rwlock_t cache_lock;

/*
 * Function: init_cache
 * This function initializes cache & cache lock
 */
void init_cache() {
	debug_printf("Inside init_cache\n");
	/* Initialize read write cache lock */
    int status = pthread_rwlock_init(&cache_lock, 0);
    if (status != 0)
    	fprintf(stderr, "Read write lock initialization error!\n");
    debug_printf("Returning from init_cache\n");
}

/*
 * Function: add_cache_node
 * This function adds new node to cache as head & calls evict_cache_node
 * function if cache size was exceeded max value with this new addition
 */
void add_cache_node(cache_list *cache, char *path, char *content, \
					unsigned int content_size) {
	debug_printf("Inside add_cache_node\n");
    /* Apply write lock */
    debug_printf("Applying write lock\n");
    int status = pthread_rwlock_wrlock(&cache_lock);
    if (status != 0 && status != EDEADLK)
    	fprintf(stderr, "Cannot apply write lock in add_cache_node!\n");
    						
    /* If new node addition exceeds max cache size node 
       eviction is done to ensure cache size is less than
       max allowed value */
    debug_printf("Evicting nodes if cache size exceeds\n");
    while((cache->total_size + content_size) > MAX_CACHE_SIZE)
    	evict_cache_node(cache);
    
    /* Increment global time tracker variable */
    time_counter++;
    
    /* Allocate memory for new node */
    debug_printf("Allocating memory to new node\n");
    cache_node *new_node = Malloc(sizeof(cache_node));
    
    /* Update the contents of new node */
    debug_printf("Updating contents of new node\n");
    new_node->content = Malloc(MAX_OBJECT_SIZE);
    memcpy(new_node->content, content, content_size);
    new_node->time = time_counter;
    new_node->path = Malloc(MAXLINE);
    strcpy(new_node->path, path);
    new_node->content_size = content_size;
    
    /* Update cache size */
    cache->total_size += content_size;

    /* New node will be added as head of cache, this will also ensure 
       recent content is returned faster */
    new_node->next_node = cache->head_node;
    cache->head_node = new_node;  
	
	/* Release lock */
	debug_printf("Releasing write lock\n");
    status = pthread_rwlock_unlock(&cache_lock);
    if (status != 0)
    	fprintf(stderr, "Unlock error in add_cache_node!\n");
    debug_printf("Leaving add_cache_node\n");
}

/*
 * Function: search_cache
 * This function traverse through linked list while searching for path
 * if paths are same, that particular node pointer is returned
 */
cache_node *search_cache(cache_list *cache, char *path) {
	debug_printf("Inside search_cache\n");
     /* Apply read lock */
    debug_printf("Applying read lock\n");
    int status = pthread_rwlock_rdlock(&cache_lock);
    if (status != 0)
    	fprintf(stderr, "Cannot apply read lock in search_cache!\n");
    
    /* Cache node pointer to traverse through cache */
   	cache_node *pointer = cache->head_node;
   	
    /* Increment global time tracker variable */
    time_counter++;
	
	debug_printf("Starting search\n");
    while(pointer != NULL) {
        /* Check path */
    	if(!strcmp(pointer->path, path)) {
    		/* Update time of node as it is read */
            pointer->time = time_counter;
            return pointer;
        }
        /* Go to next node */
        pointer = pointer->next_node;
    }
    
    /* Release lock */
    debug_printf("Releasing read lock\n");
    status = pthread_rwlock_unlock(&cache_lock);
    if (status != 0)
    	fprintf(stderr, "Unlock error in search_cache!\n");
    debug_printf("Leaving init_cache\n");
    return NULL;
}

/*
 * Function: evict_cache_node
 * This function evicts the object from cache using LRU technique
 */
void evict_cache_node(cache_list *cache) {
	debug_printf("Inside evict_cache_node\n");
     /* Apply write lock */
    debug_printf("Applying write lock\n");
    int status = pthread_rwlock_wrlock(&cache_lock);
    if (status != 0 && status != EDEADLK)
    	fprintf(stderr, "Cannot apply write lock in evict_cache_node!\n");
	
	/* Cache node pointer to traverse through cache 
		& consider least time as time of head node*/
    cache_node *pointer = cache->head_node;
    unsigned int least_time = cache->head_node->time;
	
	debug_printf("Searching for node to evict (using LRU)\n");
    while(pointer != NULL)
    {
        if(pointer->time < least_time)
            least_time = pointer->time;
        pointer = pointer->next_node;
    }

    /* Cache node pointer to traverse through cache,
    	this time for deleting the appropriate node */
    pointer = cache->head_node;
	
	debug_printf("Evicting node\n");
    cache_node *temp_node;
    /* If node to be evicted is at head */
    if( cache->head_node->time == least_time)
    {
    	debug_printf("Evicting node at head\n");
        temp_node = cache->head_node;
        cache->head_node = cache->head_node->next_node;
        cache->total_size -= temp_node->content_size;
        
        /* Free allocated memory */
        free(temp_node->content);
        free(temp_node->path);
        free(temp_node);
    }

    /* If node to be evicted is not at head */
    while(pointer->next_node != NULL)
    {	
    	debug_printf("Evicting non-head node\n");
        if(pointer->next_node->time == least_time)
        {
            temp_node = pointer->next_node;
            pointer->next_node = pointer->next_node->next_node;
            cache->total_size -= temp_node->content_size;
            
            /* Free allocated memory */
        	free(temp_node->content);
        	free(temp_node->path);
        	free(temp_node);

            return;
        }

        pointer = pointer->next_node;
    }
    
    /* Release lock */
    debug_printf("Releasing write lock\n");
    status = pthread_rwlock_unlock(&cache_lock);
    if (status != 0)
    	fprintf(stderr, "Unlock error in evict_cache_node!\n");
	debug_printf("Returning from evict_cache_node\n");
}