#include "stdio.h"
#include "time.h"
#include "string.h"
int main(){
	char src[50] = "f:hello.txt:27, d:.:26, ";
	char dst[20],c;
	int num;
	int count = sscanf(src, "%c:%s:%d", &c, dst, &num);
	printf("%s\n", dst);
	printf("%d\n", count);
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