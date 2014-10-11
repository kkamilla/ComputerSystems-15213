#include "csapp.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int globalsum=0;
int counter=0;

void handler1(int sig){

printf("the signal was recieved and the sum is%d",globalsum);
exit(0);

}

int main(int argc, char **argv){
char *p;
int i=0;
char str[128];
strcpy(str, argv[1]);
if(argc!=2){
printf("enter string\n");
exit(0);
}

char num;
//p = str;
int j = strlen(str);
printf("%d\n", j);
for(i = 0; i < j-1; i++){
        num = str[i];
	printf("%c %d \n", num, isdigit(num));
       if(isdigit(num) != 0){
	printf("atoi %c %d\n", num, atoi(&num));
     // if(atoi(&num)>0 && atoi(&num) < 10){
        globalsum= globalsum+ atoi(&num);
    }
   //p++;
}

printf("sum= %d\n",globalsum);
/*
while(1){

if(signal(SIGINT,handler1)== SIG_ERR)
	unix_error("signal error");

}
*/
return 0;
}
