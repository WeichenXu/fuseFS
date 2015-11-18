// File system check
// Assignment 2 for OS
// Author: Weichen Xu, wx431@nyu.edu
// Date: 11/16/2015
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


// environment attributes
#define BLOCK_FILE_NAME "./fusedata."
#define SUPER_BLOCK_NUMBER 0
#define MAX_BLOCK_PATH_LENGTH 20
#define MAX_BLOCK_NUM_LENGTH 10
#define MAX_FILE_NAME_LENGTH 50
#define BLOCK_SIZE 4096
#define MAX_INDIRECT_BLOCK_NUM 400
#define INVALID_TIME(t) t > (time_t)time(NULL) 
enum fs_types {directory, file};
typedef enum fs_types FILE_TYPE;
// fs_time_wcx
// time sturct for fs system
// atime: access time
// ctime: creation time
// mtime: modification time
struct fs_time_wcx
{
	time_t atime;
	time_t ctime;
	time_t mtime;
};
typedef struct fs_time_wcx FS_TIME;

// fs_entry
// entry inside directory inode
struct fs_entry_wcx
{
	FILE_TYPE type;
	char fileName[MAX_FILE_NAME_LENGTH];
	int inodeNumber;
};
typedef struct fs_entry_wcx FS_ENTRY;

// Inode
// Basic inode info struct for both file & directory
struct inode_wcx
{
	FILE_TYPE type;
	size_t size;
	int uid, gid, mode;
	int linkCount;
	FS_TIME timeInfo;
};
typedef struct inode_wcx INODE;
// Define dic_inode and file_inode inherited from inode
typedef struct
{
	INODE inode_basic;
	FS_ENTRY *dirRootEntry;
}DIR_INODE;
typedef struct{
	INODE inode_basic;
	int indirect;
	int location;
	int indirectBlockLength;
	int *blockPointers;
}FILE_INODE;

// fs_wcx
// fs struct
struct fs_wcx
{
	size_t creationTime;
	int mounted;
	int devId;
	int freeStart, freeEnd, root;
	int maxBlocks;
	//DIR_INODE *rootDir;
};
typedef struct fs_wcx FS;

int ReadFile(int blockId);
int CheckDir(DIR_INODE *dInode);

// Load the indirect location data
// return number of CSV vector, like: 1, 3, *** 6, 5
// return 0, means this is not indirect location block
int LoadIndirectLocation(char *buffer, FILE_INODE *fInode){
	int indirectCount = 0, *blockPointers = (int *)malloc(sizeof(int)*MAX_INDIRECT_BLOCK_NUM);
	// malloc faild
	char *tok;
	const char comma[2] = ", ";
	tok = strtok(buffer, comma);
	while(tok != NULL){
		indirectCount += sscanf(tok, "%d", blockPointers[indirectCount]);
		tok = strtok(NULL, comma);
	}
	fInode->indirectBlockLength = indirectCount;
	fInode->blockPointers = realloc(blockPointers, sizeof(int)*indirectCount);
	// if realloc failed
	return indirectCount;
}

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

// read n th block into buffer
// load the 4096 bytes of block into buffer
int ReadNthBlock(int n, char* buffer){
	char ch;
	char blockPath[MAX_BLOCK_PATH_LENGTH];
	FILE *p;
	int count = 0;
	BlockPathWithIndex(n, blockPath);	// get block file path
	p = fopen(blockPath, "r");
	if(p == NULL){	// whether file is opened
		printf("%s %s\n", "Unable to open the file: ", blockPath);
		return -1;
	}
	while( (ch = fgetc(p)) != EOF) {
    	buffer[count] = ch;
    	count++;
 	}
 	buffer[count] = '\0';	// set '\0' to end of buffer
	fclose(p);
	return 0;
}

// Load fs info from super block
// Check super block format if correct or not
// Check DevId
// Check creation time is valid
int LoadFS(FS *fs){
	char buffer[BLOCK_SIZE];
	ReadNthBlock(SUPER_BLOCK_NUMBER, buffer);
	int Count = sscanf(buffer,
		"{creationTime: %lu, mounted: %d, devId:%d, freeStart:%d, freeEnd:%d, root:%d, maxBlocks:%d}",
		&fs->creationTime, &fs->mounted, &fs->devId, &fs->freeStart, &fs->freeEnd, &fs->root, &fs->maxBlocks
		);
	if(Count != 7){
		printf("%s\n", "Load Super Block Failed\n");
		return -3;
	}
	// check whether the fs DevId is valid
	if(fs->devId != 20){
		printf("%d is invalid fs DevId\n", fs->devId);
		return -1;
	}
	// check creation time
	if(INVALID_TIME(fs->creationTime)){
		printf("File system creation time is in the future\n");
		return -2;
	}
	return 0;
}
// Print Inode Basic info for debug
void PrintInode(INODE* toPrint){
	printf("%lu %d %ld\n", toPrint->size, toPrint->uid, toPrint->timeInfo.atime);
}
// Load basic inode info from buffer
int LoadInodeInfoFromBuffer(INODE* toSet, char* buffer){
	if(toSet->type == directory){
		int Count = sscanf(buffer,
			"{size: %lu, uid: %d, gid:%d, mode:%d, atime:%ld, ctime:%ld, mtime:%ld, linkcount:%d",
			&toSet->size, &toSet->uid, &toSet->gid, &toSet->mode, &toSet->timeInfo.atime, &toSet->timeInfo.ctime, &toSet->timeInfo.mtime,
			 &toSet->linkCount);
		if(Count != 8){
			printf("%s\n", "Load Inode Basic Info Failed\n");
			return -1;
		}
	}
	if(toSet->type == file){
		int Count = sscanf(buffer,
			"{size: %lu, uid: %d, gid:%d, mode:%d, linkcount:%d, atime:%ld, ctime:%ld, mtime:%ld",
			&toSet->size, &toSet->uid, &toSet->gid, &toSet->mode, &toSet->linkCount, &toSet->timeInfo.atime, &toSet->timeInfo.ctime, 
			&toSet->timeInfo.mtime
			);
		if(Count != 8){
			printf("%s\n", "Load Inode Basic Info Failed\n");
			return -1;
		}
	}
	PrintInode(toSet);
	return 0;
}

// set link entry
// f:name.txt:122
int setLinkEntry(FS_ENTRY *entry, char *buffer, int entryNum){
	char *tok, entryType;
	const char comma = ':';
	const char semi[2] = ",";
	int count = 0;
	tok = strtok(buffer, &comma);
	while(tok != NULL){
		printf("tok is: %s\n", tok);
		entryType = *(tok+strlen(tok)-1);
		//printf("Entry type: %c\n", entryType);
		if(entryType == 'f'){
			entry[count].type = file;
		}
		if(entryType == 'd'){
			entry[count].type = directory;
		}
		//entry[count].type = *(tok+strlen(tok)-1);
		tok = strtok(NULL, &comma);
		sscanf(tok, "%s", entry[count].fileName);
		tok = strtok(NULL, semi);
		sscanf(tok, "%d",&entry[count].inodeNumber);
		tok = strtok(NULL, &comma);
		//printf("%c %s %d\n", entry[count].type, entry[count].fileName, entry[count].inodeNumber);
		count++;
		if(count > entryNum){
			printf("%s\n", "Entry number exceeds the number in the inode");
			return -1;
		}
	}
	if(count < entryNum){
		printf("Entry number is fewer than directory link count!\n");
		return -2;
	}
	return 0;
}

// Load link entries for DIR_INODE
// linkcount:4, filename_to_inode_dict:  {f:foo:1234 ...
int LoadLinkEntry(DIR_INODE *dInode, char* buffer){
	char *entryStart;
	// make sure "linkCount" && "filename_to_inode_dict"
	// are correctly existed in the inode
	if(!(entryStart = strstr(buffer, "filename_to_inode_dict"))){
		printf("%s\n", "Invalid Dir Inode format: can not load links");
		return -1;
	}
	//setCount1 = sscanf(linkStart, "linkcount:%d", &tempLinkCount);	// get the link count
	//dInode->linkCount = tempLinkCount;
	//if(setCount1 != 1){	printf("%s\n", "Set linkCount Failed"); return -2;}
	printf("linkCount:%d\n", dInode->inode_basic.linkCount);

	// alllocate space for dirRootEntry
	dInode->dirRootEntry = (FS_ENTRY*) malloc(dInode->inode_basic.linkCount*sizeof(FS_ENTRY));
	entryStart += sizeof("filename_to_inode_dict: {") - 1; 	// move the start of the buffer after '{'
	setLinkEntry(dInode->dirRootEntry, entryStart, dInode->inode_basic.linkCount);
	return 0;
}

// Load file entry
// indirect: 0/1 location:xxxx
int LoadFileEntry(FILE_INODE *fInode, char *buffer){
	char *indirectTok, *locationTok;
	indirectTok = strstr(buffer, "indirect:");
	indirectTok += strlen("indirect:");
	sscanf(indirectTok, "%d", &fInode->indirect);
	locationTok = strstr(buffer, "location:");
	locationTok += strlen("location:");
	sscanf(locationTok, "%d", &fInode->location);
	printf("indirect:%d location:%d\n", fInode->indirect, fInode->location);
	return 0;
}

// Read directory INODE
int ReadDir(int blockId){
	int valid;
	printf("Read Dir block num:%d\n", blockId);
	char buffer[BLOCK_SIZE];
	DIR_INODE *dInode = (DIR_INODE*)malloc(sizeof(DIR_INODE));
	dInode->inode_basic.type = directory;
	ReadNthBlock(blockId, buffer);
	LoadInodeInfoFromBuffer(&dInode->inode_basic, buffer);	// load basic info to Inode
	LoadLinkEntry(dInode, buffer);	// load link entries into Inode
	if((valid = CheckDir(dInode))){
		return -1;
	}
	// free the DIR_INODE
	if(dInode->dirRootEntry)	free(dInode->dirRootEntry);
	free(dInode);
	
	return 0;
}

/*--------------------------------------------------------------------------------------*/
/* check part:
 * check Dir
 * check file, indirect & size
 * check free blocks
*/
// check whether time are in the future
 int CheckTime(FS_TIME t){
 	if(INVALID_TIME(t.atime)){
 		printf("Invalid atime\n");
 		return -1;
 	}
 	if(INVALID_TIME(t.mtime)){
 		printf("Invalid mtime\n");
 		printf("%ld\n", t.mtime);
 		return -2;
 	}
 	if(INVALID_TIME(t.ctime)){
 		printf("Invalid ctime\n");
 		return -3;
 	}
 	return 0;
 }
 // linkcount has been checked when readDir
 // check whether the dir has '.' && '..'
int CheckDir(DIR_INODE *dInode){
	//printf("start check Dir\n");
	//printf("Check Dir entry: %s\n", dInode->dirRootEntry[0].fileName);
	int currentDirExist = 0, parentDirExist = 0;
	for(int i=0; i<dInode->inode_basic.linkCount; i++){
		printf("InodeIndex: %d, Inode_type:%d, Inode_fileName:%s\n", i, dInode->dirRootEntry[i].type, dInode->dirRootEntry[i].fileName);
		if(dInode->dirRootEntry[i].type == file){
			ReadFile(dInode->dirRootEntry[i].inodeNumber);	//  load and check the file
		}
		if(dInode->dirRootEntry[i].type == directory){
			if(!strcmp(dInode->dirRootEntry[i].fileName,"..")){
				parentDirExist = 1;
				continue;
			}
			if(!strcmp(dInode->dirRootEntry[i].fileName,".")){
				currentDirExist = 1;
				continue;
			}
			ReadDir(dInode->dirRootEntry[i].inodeNumber);
		}
	}
	
	if(!currentDirExist){
		printf("Current Dir miss!\n");
		return -1;
	}
	if(!currentDirExist){
		printf("Parent Dir miss!\n");
		return -2;
	}
	if(CheckTime(dInode->inode_basic.timeInfo)){
		return -3;
	}
	return 0;
}


// check file
// time is valid
// location is vector or not
// check size
int CheckFile(FILE_INODE *fInode){
	int valid;
	char buffer[BLOCK_SIZE];
	if((valid = CheckTime(fInode->inode_basic.timeInfo))){
		return -1;
	}
	ReadNthBlock(fInode->location, buffer);

	return 0;
}

// Read file INODE
int ReadFile(int blockId){
	printf("Read file block Id: %d\n", blockId);
	char buffer[BLOCK_SIZE];
	FILE_INODE *fInode = (FILE_INODE*)malloc(sizeof(FILE_INODE));
	fInode->inode_basic.type= file;
	ReadNthBlock(blockId, buffer);
	LoadInodeInfoFromBuffer(&fInode->inode_basic, buffer); // load basic info to Inode
	LoadFileEntry(fInode, buffer);

	CheckFile(fInode);

	free(fInode);
	return 0;
}
int main(){
	int valid;
	FS *fs = (FS*)malloc(sizeof(FS));
	if((valid = LoadFS(fs))){	// load super block and check
		printf("Super block invalid\n");
		return 0;
	}
	if((valid = ReadDir(fs->root))){	// load root dir and check
		printf("Read root dir failed\n");
		return 0;
	}
	free(fs);
}