#include "stdio.h"
#include "time.h"
#include "string.h"
#include "stdlib.h"
typedef struct {
	int a;
	int b;
}two;
int main(){
	char buffer[100] = "f:hello.txt:27, d:.:26, d:..:25, d:test:30}}";
	char *tok;
	const char comma = ':';
	const char semi[2] = ",";
	int count = 0;
	tok = strtok(buffer, &comma);
	while(tok != NULL){
		printf("tok is: %s\n", tok);
		printf("Entry type: %c\n", *(tok+strlen(tok)-1));
		/*
		if(*(tok+strlen(tok)-1) == 'f'){
			entry[count].type = file;
		}
		if(*(tok+strlen(tok)-1) == 'd'){
			entry[count].type = directory;
		}
		*/
		//entry[count].type = *(tok+strlen(tok)-1);
		tok = strtok(NULL, &comma);
		printf("tok is: %s\n", tok);
		tok = strtok(NULL, semi);
		printf("tok is: %s\n", tok);
		tok = strtok(NULL, &comma);
		//printf("%c %s %d\n", entry[count].type, entry[count].fileName, entry[count].inodeNumber);
		
	}
	return 0;
	/*
	two *test = (two*) malloc(sizeof(two) * 5);
	test[1].a = 5;
	printf("%d\n", test[1].a);
	free(test);
	*/
	/*
	char src[100] = "1, 2, 3, 4";
	char *tok1;
	const char comma[2] = ", ";
	int count = 0, num;
	tok1 = strtok(src, comma);
	while(tok1 != NULL){
		sscanf(tok1, "%d", &num);
		printf("Number %d id is %s\n", count, tok1);
		tok1 = strtok(NULL, comma);
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