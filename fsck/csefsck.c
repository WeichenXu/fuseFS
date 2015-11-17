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
	char type;
	char fileName[MAX_FILE_NAME_LENGTH];
	int inodeNumber;
};
typedef struct fs_entry_wcx FS_ENTRY;
// fs_wcx
// fs struct
struct fs_wcx
{
	size_t creationTime;
	int mounted;
	int devId;
	int freeStart, freeEnd, root;
	int maxBlocks;
};
typedef struct fs_wcx FS;

// Inode
// Basic inode info struct for both file & directory
struct inode_wcx
{
	FILE_TYPE type;
	size_t size;
	int uid, gid, mode;
	FS_TIME timeInfo;
};
typedef struct inode_wcx INODE;
// Define dic_inode and file_inode inherited from inode
typedef struct
{
	INODE inode_basic;
	int linkCount;
	FS_ENTRY *dirRootEntry;
}DIR_INODE;
typedef struct{
	INODE inode_basic;
	int indirect;
	int location;
}FILE_INODE;
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
	if(fs->creationTime >= (time_t)time(NULL)){
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
	int Count = sscanf(buffer,
		"{size: %lu, uid: %d, gid:%d, mode:%d, atime:%ld, ctime:%ld, mtime:%ld",
		&toSet->size, &toSet->uid, &toSet->gid, &toSet->mode, &toSet->timeInfo.atime, &toSet->timeInfo.ctime, &toSet->timeInfo.mtime
		);
	if(Count != 7){
		printf("%s\n", "Load Inode Basic Info Failed\n");
		return -1;
	}
	//PrintInode(toSet);
	return 0;
}

// set link entry
// f:name.txt:122
int setLinkEntry(FS_ENTRY *entry, char *buffer, int entryNum){
	char *tok;
	const char comma[1] = ":";
	const char semi[2] = ",}";
	int count = 0;
	tok = strtok(buffer, comma);
	while(tok != NULL){
		entry[count].type = *(tok+strlen(tok)-1);
		tok = strtok(NULL, comma);
		sscanf(tok, "%s", entry[count].fileName);
		tok = strtok(NULL, semi);
		sscanf(tok, "%d",&entry[count].inodeNumber);
		tok = strtok(NULL, comma);
		printf("%c %s %d\n", entry[count].type, entry[count].fileName, entry[count].inodeNumber);
		count++;
		if(count > entryNum){
			printf("%s\n", "Entry number exceeds the number in the inode");
		}
	}
	return 0;
}

// Load link entries for DIR_INODE
// linkcount:4, filename_to_inode_dict:  {f:foo:1234 ...
int LoadLinkEntry(DIR_INODE *dInode, char* buffer){
	char *linkStart, *entryStart;
	int tempLinkCount, setCount1;
	// make sure "linkCount" && "filename_to_inode_dict"
	// are correctly existed in the inode
	if(!(linkStart = strstr(buffer, "linkcount")) || !(entryStart = strstr(buffer, "filename_to_inode_dict"))){
		printf("%s\n", "Invalid Dir Inode format: can not load links");
		return -1;
	}
	setCount1 = sscanf(linkStart, "linkcount:%d", &tempLinkCount);	// get the link count
	dInode->linkCount = tempLinkCount;
	if(setCount1 != 1){	printf("%s\n", "Set linkCount Failed"); return -2;}
	printf("linkCount:%d\n", tempLinkCount);
	// alllocate space for dirRootEntry
	dInode->dirRootEntry = (FS_ENTRY*) malloc(tempLinkCount*sizeof(FS_ENTRY));
	entryStart += sizeof("filename_to_inode_dict: {") - 1; 	// move the start of the buffer after '{'
	setLinkEntry(dInode->dirRootEntry, entryStart, tempLinkCount);
	/*
	for(i=0; i<tempLinkCount; i++){
		setCount2 = sscanf(entryStart, "%c:%s:%d", &entryType, entryFileName, &entryInodeNumber);
		strncpy(&(dInode->dirRootEntry[i].type), &entryType, sizeof(char));
		strncpy(dInode->dirRootEntry[i].fileName, entryFileName, strlen(entryFileName)+1);
		dInode->dirRootEntry[i].inodeNumber = entryInodeNumber;
		//&(dInode->dirRootEntry[i].type), &(dInode->dirRootEntry[i].fileName), &(dInode->dirRootEntry[i].inodeNumber));
		if(setCount2 != 3){
			printf("Invalid linkEntry Format in %s\n", entryStart);
			printf("set %d attributes in entry\n", setCount2);
			return -3;
		}
		entryStart += sizeof(char) + strlen(entryFileName) + sizeof(int) + 4; // move the ptr to next entry
		printf("Entry %d is: %c %s %d", i, entryType, entryFileName, entryInodeNumber);
	}
	*/
	return 0;
}

// Read directory INODE
int ReadDir(int blockId){
	char buffer[BLOCK_SIZE];
	DIR_INODE *dInode = (DIR_INODE*)malloc(sizeof(DIR_INODE));
	ReadNthBlock(blockId, buffer);
	LoadInodeInfoFromBuffer(&dInode->inode_basic, buffer);	// load basic info to Inode
	LoadLinkEntry(dInode, buffer);	// load link entries into Inode
	// free the DIR_INODE
	if(dInode->dirRootEntry)	free(dInode->dirRootEntry);
	free(dInode);
	
	return 0;
}

// Read file INODE
int ReadFile(int blockId){
	char buffer[BLOCK_SIZE];
	FILE_INODE *fInode = (FILE_INODE*)malloc(sizeof(FILE_INODE));
	ReadNthBlock(blockId, buffer);
	LoadInodeInfoFromBuffer(&fInode->inode_basic, buffer); // load 
}
int main(){
	FS *fs = (FS*)malloc(sizeof(FS));
	LoadFS(fs);
	ReadDir(fs->root);
	free(fs);
}