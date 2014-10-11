
//Krutika Kamilla
//Andrew ID- kkamilla

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"
#include <math.h>
int hit=0;
 int miss=0;
 int eviction=0;


//structure for one line
typedef struct line {
int valid;
int tag;
int lru;//store access sequence and highest value represents the most recent
}line;

void cache_func(int address, int s, int b, int E, struct line** cache)
{
int mask,temp,setBits,tagBits,i,minlru,minIndex;
int full=1; 
//set bits found by shifting the address to block bits, then & with mask
mask=0x7fffffff >> (31 - s);
temp=address >> b;
setBits=mask & temp;

//tag bits are obtained by right shifting address by block+set bits, then & with mask
mask=0x7fffffff >> (31 - s - b);
temp=address >> (s + b);
tagBits=mask & temp;

for (i=0; i<E; i++)
    {
        if (cache[setBits][i].valid == 0)
            full = 0;
        if (cache[setBits][i].valid == 1 && cache[setBits][i].tag == tagBits)
        {
            //it is hit so increasing hit counter
            cache[setBits][i].lru += 1;
            hit += 1;
            return;
            }
    }

//If there was no hit then it's miss
miss += 1;
//In case of eviction the item is removed from memory. If the cache is full, Least recently used policy is used else empty line is replaced.
if(full)
{
eviction += 1;
//Find out the line which is least recently used
minlru=cache[setBits][0].lru;
minIndex=0;
for (i=0; i<E; i++)
{
if(cache[setBits][i].lru < minlru)
{
 minlru=cache[setBits][i].lru;
 minIndex=i;
 }
 }
      cache[setBits][minIndex].tag=tagBits;                                                                               cache[setBits][minIndex].lru +=1;                                                                             }
 //If cache is not full then empty cache line can be checked to overwrite contents
   else{                                                                                                                      for (i=0; i<E; i++) 
                { 
                   if(cache[setBits][i].valid == 0)                                                                                      { 
                     cache[setBits][i].valid = 1;
                     cache[setBits][i].lru += 1; 
                     cache[setBits][i].tag = tagBits;
                     return;
                    }
                 }
       }
}

int main(int argc, char **argv)
{
	int opt;
        int h = 0;//help flag 
	int v = 0;//verbose flag
	int s = 0;
	int b = 0;
	int E = 0;
        char operation; //to store operation type in tracefile
	int i;
	int numberOfSets;//total number of sets
        int address,size;//address & size in  trace file
	char *tracefile = "";
        FILE *traceFile;
        while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
             switch (opt) {
			case 'h':
				h = 1;
				break;
			case 'v':
				v = 1;
				break;
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				tracefile = optarg;
				break;
			default:
				break;
		}
	}
	if(h == 1) {printf(
"Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n");
		return 0;
	}

numberOfSets=(2<<(s-1));

//Initializing cache memory
  struct line** cache = (struct line**) malloc(numberOfSets * E * sizeof(struct line*));
    for (i=0;i < numberOfSets;i++)
      {
     cache[i]=(struct line*) malloc(E * sizeof(struct line));
       cache[i]->valid=0;
       cache[i]->tag=0;
       cache[i]->lru=0;
       }


traceFile=fopen(tracefile,"r");
    if (traceFile == NULL)
    {
        printf("No contents in trace file.");
        return 1;
    }
    
    //Recursion through trace file
  while (fscanf(traceFile, " %c %x,%d", &operation, &address, &size) > 0)
       {
   //Go to next iteration if operation is I
    if (operation == 'I')
    continue;
   //call simulation function once if operation type is S or L
    if (operation == 'S' || operation == 'L')
    cache_func(address,s,b,E,cache);
  //call simulation function twice if operation type is M i.e. eviction, because item is replaced from cache and then stored.
   if (operation == 'M')
   {
    cache_func(address,s,b,E,cache);
    cache_func(address,s,b,E,cache);
     }
    }
  fclose(traceFile);
  printSummary(hit,miss,eviction);
   //Free memory allocated using malloc
  for (i = 0; i < E; i++)
  {
                                                                                                                                                               free(cache[i]);                                                                                                                                                                }
   free(cache);                                                                                                                                                  return 0;
}

