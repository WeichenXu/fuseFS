#include "stdio.h"
#include "time.h"
#include "string.h"
#include "stdlib.h"
typedef struct {
	int a;
	int b;
}two;
int main(){
	two *test = (two*) malloc(sizeof(two) * 5);
	test[1].a = 5;
	printf("%d\n", test[1].a);
	free(test);
	/*
	char src[100] = "f:hello.txt:27, d:.:26, d:..:27}";
	char dst[20],c, *tok1;
	const char comma[1] = ":";
	const char semi[2] = ",}";
	int num, count = 0;
	tok1 = strtok(src, comma);
	while(tok1 != NULL){
		c = *(tok1+strlen(tok1)-1);
		tok1 = strtok(NULL, comma);
		sscanf(tok1, "%s", dst);
		tok1 = strtok(NULL, semi);
		sscanf(tok1, "%d",&num);
		tok1 = strtok(NULL, comma);
		printf("%c %s %d\n", c, dst, num);
	}
	*/
	/*
	int num;
	int count = sscanf(src, "%c:%s:%d", &c, dst, &num);
	printf("%s\n", dst);
	printf("%d\n", count);
	*/
	/*
	strncpy(dst, src, strlen(src));
	printf("%lu\n", strlen(dst));
	printf("%s\n", dst);
	*/
	// test time
	//printf("Current Time: %lu\n", (time_t)time(NULL));
	//printf("Current Time: %ld\n", (time_t)time(NULL));

	return 0;
}