#include "stdio.h"
#include "time.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"
#define MAX_BLOCK_PATH_LENGTH 20
#define MAX_BLOCK_NUM_LENGTH 10
#define BLOCK_FILE_NAME "./fusedata."
typedef struct {
	int a;
	int b;
}two;
// get block file path
// : fusedata.*
void BlockPathWithIndex(int n, char* path){
	char blockNum[MAX_BLOCK_NUM_LENGTH];
	// get block file path
	strncpy(path, BLOCK_FILE_NAME, sizeof(BLOCK_FILE_NAME));
	sprintf(blockNum, "%d", n);
	strncat(path, blockNum, strlen(blockNum));
	// return the char* "*data.*"
}
void resize( int **p, int size ) {
   free( *p );
   *p = (int*) malloc( size * sizeof(int) );
   (*p)[0] = 1;
   printf("%d\n",*p[2]);
}


void test(int **a){
	//printf("%d\n",a);
	//if(*a) free(*a);
	*a = (int *)malloc(sizeof(int)*2);
	(*a)[1] = 1;
	(*a)[0] = 2;
	//*a = (int *)realloc(a, sizeof(two)*2);
	printf("%d\n",(*a)[1]);
	//return a;
}
int main() {
   int *p = NULL;//(int*) malloc( 10 * sizeof(int) );
   test( &p );
   printf("%d\n", p[0]);
   if(p) free(p);
   //return 0;
}
/*
int main(){
	int *a = NULL;
	resize(&a,3);
	printf("%d\n",a[1]);
	free(a);

	return 0;
}
*/
	/*
	bool array[100] = {false};
	for(int i=0; i<5; i++){
		if(array[i] == false)	printf("it is false in %d\n", i);
	}
	*/
	/*
	two* a = NULL, *b;
	for(int i=0; i<5; i++){
		a = realloc(a, sizeof(two)*(i+1));
		a[i].a = 1;
		a[i].b = 2;
	}
	b = a;
	for(int j=0; j<5; j++){
		printf("b[%d] is %d %d\n", j, b[j].a, b[j].b);
	}
	*/
	/*
	char buffer[100] = "f:hello.txt:27, d:.:26, d:..:25, d:test:30}}";
	char *tok;
	char blockPath[MAX_BLOCK_PATH_LENGTH];
	FILE *p;
	BlockPathWithIndex(88, blockPath);	// get block file path
	p = fopen(blockPath, "w");
	if(p == NULL){	// whether file is opened
		printf("%s %s\n", "Unable to open the file: ", blockPath);
		return -1;
	}
	fwrite(buffer, sizeof(char), strlen(buffer), p);
	fclose(p);
	*/
	/*
	const char comma = ':';
	const char semi[2] = ",";
	int count = 0;
	tok = strtok(buffer, &comma);
	while(tok != NULL){
		printf("tok is: %s\n", tok);
		printf("Entry type: %c\n", *(tok+strlen(tok)-1));
		
		//entry[count].type = *(tok+strlen(tok)-1);
		tok = strtok(NULL, &comma);
		printf("tok is: %s\n", tok);
		tok = strtok(NULL, semi);
		printf("tok is: %s\n", tok);
		tok = strtok(NULL, &comma);
		//printf("%c %s %d\n", entry[count].type, entry[count].fileName, entry[count].inodeNumber);
		
	}
	return 0;
	*/
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
