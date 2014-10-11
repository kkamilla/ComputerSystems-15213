#include<stdio.h>
#include<stdlib.h>
#include "csapp.h"

int globalcounter=0;
int globalcounter2=0;
int globalsum=0;

void handler(int sig){
printf("the sum is %d\n",globalsum);
exit(0);
}

int main(int argc, char* argv[]){
int len=strlen(argv[1]);
char buf[len];
strcpy(buf,argv[1]);
char* p= &buf[0];
while(len>0){
char temp =*p;
if((int)temp>=48 && (int)temp <=57){
	globalsum+=atoi(&temp);
	globalcounter++;

}
p++;
globalcounter2++;
len--;
}
printf("global sum=%d",globalsum);
while(1){
if(signal(SIGINT,handler)== SIG_ERR){
	printf("signal detected");
}
return 0;
}

}
