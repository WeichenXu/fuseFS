// FUSE File system check
// Assignment 2 for OS
// Author: Weichen Xu, wx431@nyu.edu
// Date: 11/16/2015
//
// Compile: gcc -Wall -o csefsck csefsck.c
// Run: ./csefsck
// 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

// environment attributes
#define BLOCK_FILE_NAME "./fusedata."
#define SUPER_BLOCK_NUMBER 0
#define MAX_BLOCK_PATH_LENGTH 20
#define MAX_BLOCK_NUM_LENGTH 10
#define MAX_FILE_NAME_LENGTH 50
#define BLOCK_SIZE 4096
#define MAX_INDIRECT_BLOCK_NUM 400
#define MAX_ENTRY_PER_DIR 500
#define INVALID_TIME(t) t > (time_t)time(NULL) // whether time is in the future
#define SET_TO_CURRENT_TIME(t) t = (time_t)time(NULL)
// two types of inodes in fs
enum fs_types {directory, file};
typedef enum fs_types FILE_TYPE;

// fs_time_wcx
// time sturct for fs system
typedef struct fs_time_wcx
{
	time_t atime;
	time_t ctime;
	time_t mtime;
}FS_TIME;

// fs_entry
// entry inside directory inode
typedef struct fs_entry_wcx
{
	FILE_TYPE type;
	char fileName[MAX_FILE_NAME_LENGTH];
	int inodeNumber;
}FS_ENTRY;

// Inode
// Basic inode info struct for both file & directory
typedef struct inode_wcx
{
	FILE_TYPE type;
	size_t size;
	int uid, gid, mode;
	int linkCount;
	FS_TIME timeInfo;
}INODE;
// Define dic_inode and file_inode inherited from inode
typedef struct
{
	char dirName[MAX_FILE_NAME_LENGTH];
	int inodeNumber, parent_inodeNumber;
	INODE inode_basic;
	FS_ENTRY *dirRootEntry;
}DIR_INODE;

typedef struct{
	char fileName[MAX_FILE_NAME_LENGTH];
	int inodeNumber;
	INODE inode_basic;
	int indirect,location;
	int locationArrayLength,*locationArray;
}FILE_INODE;

// fs_wcx
// fs struct
typedef struct fs_wcx
{
	size_t creationTime;
	int mounted;
	int devId;
	int freeStart, freeEnd, root;
	int maxBlocks, freeBlockLength;
	bool *blockMap;
	// like bit map for checking
	// true: free
	// false: in use
}FS;

//----------------------------------------------------------------------------

// Input/Output to Block
void BlockPathWithIndex(int n, char* path);
int ReadNthBlock(int n, char* buffer);
int WriteNthBlock(int n, char* buffer, int size);

// Load Inode, location array, file/dir entry
int LoadInodeInfoFromBuffer(INODE* toSet, char* buffer);
int LoadLinkEntry(DIR_INODE *dInode, char* buffer);
int LoadLinkEntryHelper(DIR_INODE *dInode, char *buffer);
int LoadFileEntry(FILE_INODE *fInode, char *buffer);
int LoadBlockArray(int blockId, int **blockArray);
// Write Inode, file/dir entry
int WriteDirInode(DIR_INODE *dInode);
int WriteFileInode(FILE_INODE *fInode);

// Read Dir & File
int ReadDir(FS *fs, DIR_INODE *dInode);
int ReadFile(FS *fs, FILE_INODE *fInode);

// Load & Check FS
int LoadFS(FS *fs);
int CheckTime(FS_TIME *t);
int CheckDir(FS *fs, DIR_INODE *dInode, int actualLinkCount);
int CheckFile(FS *fs, FILE_INODE *fInode);

// Load and Check Free Block List
int LoadFreeBlockList(FS *fs);
int CheckBlockValid(FS *fs, int blockId);
int CheckAllBlocks(FS *fs);
// Set and Write free block list
int WriteFreeBlockList(FS *fs);
int AssignBlock(FS *fs);
int FreeBlock(FS *fs);

//----------------------------------------------------------------------------


int CheckAllBlocks(FS *fs){
	int valid = 1;
	for(int i=0; i < fs->maxBlocks; i++){
		if(fs->blockMap[i] == true)	{
			printf("ERROR!Block: %d missing in the free block list\n", i);
			valid = 0;
		}
	}
	return valid;
}
int CheckBlockValid(FS *fs, int blockId){
	if(fs->blockMap[blockId] == true){
		fs->blockMap[blockId] = false;
		return 1;
	}
	else{
		printf("ERROR!!!!!Block: %d is already in the free block list\n", blockId);
		return 0;
	}
}
//-------------------------------------------------------------------------
// This part is about checking free block list
// LoadFreeBlockList, CheckAllBlocks
int LoadFreeBlockList(FS *fs){
	//bool allBloackFree = ture;
	int blockLength = 0, *blockArray = NULL;
	fs->freeBlockLength = 0;
	printf("Init free block map from freeStart to freeEnd\n");
	for(int i=0; i<fs->maxBlocks; i++){
		if(!i || (fs->freeStart <= i && i <= fs->freeEnd))	fs->blockMap[i] = false;
		else fs->blockMap[i] = true;
	}
	for(int i = fs->freeStart; i <= fs->freeEnd; i++){
		blockLength = LoadBlockArray(i, &blockArray);
		fs->freeBlockLength += blockLength;
		//printf("%ds blocks find in %d locaiton array\n", blockLength, i);
		for(int j=0; j<blockLength; j++){
			int blockId = blockArray[j];
			if(fs->blockMap[blockId] == false)	printf("ERROR!!!!!Block: %d is already used by super or free list block\n", blockId);
			else fs->blockMap[blockId] = false;
		}
		free(blockArray);
	}

	printf("%d free Blocks in total\n", fs->freeBlockLength);
	return 0;
}
// Write free block list back
int WriteFreeBlockList(FS *fs){

}

// Load the indirect location data
// number of CSV vector, like: 1, 3, *** 6, 5
// return 0, means this is not indirect location block
int LoadBlockArray(int blockId, int **blockArray){
	int indirectCount = 0;
	char *tok, buffer[BLOCK_SIZE];
	ReadNthBlock(blockId,buffer);
	*blockArray = (int *)malloc(sizeof(int)*MAX_INDIRECT_BLOCK_NUM);
	// incase realloc failed
	const char comma[2] = ",";
	tok = strtok(buffer, comma);
	while(tok != NULL){
		//printf("tok is: %s\n", tok);
		indirectCount += sscanf(tok, "%d", &(*blockArray)[indirectCount]);
		//printf("block is: %d\n", blockArray[indirectCount-1]);
		tok = strtok(NULL, comma);
	}
	*blockArray = realloc(*blockArray, sizeof(int)*indirectCount);
	//fInode->locationArrayLength = indirectCount;
	//fInode->locationArray = realloc(blockPointers, sizeof(int)*indirectCount);
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

// read n th block/file into buffer
// load the 4096 bytes of block into buffer
int ReadNthBlock(int n, char* buffer){
	char ch;
	char blockPath[MAX_BLOCK_PATH_LENGTH];
	FILE *p;
	int count = 0;
	BlockPathWithIndex(n, blockPath);	// get block file path
	p = fopen(blockPath, "r");
	if(p == NULL){	// whether file is opened
		printf("%s %s\n", "Unable to open the file/block for read: ", blockPath);
		return -1;
	}
	while( (ch = fgetc(p)) != EOF) {
    	buffer[count] = ch;
    	count++;
 	}
 	if(count < BLOCK_SIZE)	buffer[count] = '\0';	// set '\0' to end of buffer
	fclose(p);
	return 0;
}
// wirte n th buffer into block/file
// wirte strlen buffer into the file
int WriteNthBlock(int n, char* buffer, int size){
	char blockPath[MAX_BLOCK_PATH_LENGTH];
	FILE *p;
	BlockPathWithIndex(n, blockPath);	// get block file path
	p = fopen(blockPath, "w");
	if(p == NULL){	// whether file is opened
		printf("%s %s\n", "Unable to open the file/block for write: ", blockPath);
		return -1;
	}
	if(size >= BLOCK_SIZE)	fwrite(buffer, sizeof(char), BLOCK_SIZE, p);
	else	{
		buffer[size] = '\0';
		fwrite(buffer,sizeof(char), size+1, p);
	}
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
		printf("%s\n", "Invalid Super Block Format\n");
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
// Load free block list into the fs
// The free list starts from nth, to mth, all in CSV format



/* Print Inode Basic info for debug
void PrintInode(INODE* toPrint){
	printf("%lu %d %ld\n", toPrint->size, toPrint->uid, toPrint->timeInfo.atime);
}
*/
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
	//PrintInode(toSet);
	return 0;
}

// set link entry
// f:name.txt:122
int LoadLinkEntryHelper(DIR_INODE *dInode, char *buffer){
	char *tok, entryType;
	FS_ENTRY *entry = (FS_ENTRY*)malloc(sizeof(FS_ENTRY)*MAX_ENTRY_PER_DIR);
	const char comma[2] = ":";
	const char semi[2] = ",";
	int count = 0;
	tok = strtok(buffer, comma);
	while(tok != NULL){
		//if((count+1) > dInode->inode_basic.linkCount)	entry = (FS_ENTRY *)realloc(entry, sizeof(FS_ENTRY) * (count+1));
		entryType = *(tok+strlen(tok)-1);
		//printf("Entry type: %c\n", entryType);
		if(entryType == 'f'){
			entry[count].type = file;
		}
		if(entryType == 'd'){
			entry[count].type = directory;
		}
		//entry[count].type = *(tok+strlen(tok)-1);
		tok = strtok(NULL, comma);
		//printf("tok is: %s\n", tok);
		sscanf(tok, "%s", entry[count].fileName);
		tok = strtok(NULL, semi);
		sscanf(tok, "%d",&entry[count].inodeNumber);
		tok = strtok(NULL, comma);
		//printf("%d %s %d\n", entry[count].type, entry[count].fileName, entry[count].inodeNumber);
		count++;

	}
	entry = (FS_ENTRY *)realloc(entry, sizeof(FS_ENTRY) * count);
	dInode->dirRootEntry = entry;
	return count;
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

	// alllocate space for dirRootEntry
	//dInode->dirRootEntry = (FS_ENTRY*) malloc(dInode->inode_basic.linkCount*sizeof(FS_ENTRY));
	entryStart += sizeof("filename_to_inode_dict: {") - 1; 	// move the start of the buffer after '{'
	return LoadLinkEntryHelper(dInode, entryStart);
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
	//printf("indirect:%d location:%d\n", fInode->indirect, fInode->location);
	return 0;
}

// Write dir inode
int WriteDirInode(DIR_INODE *dInode){
	char buffer[BLOCK_SIZE], entry[BLOCK_SIZE], entryType;	// generalize inode info & dir entry, then write back to the block
	int inodeSize = 0, entrySize = 0;
	INODE *toSet;
	if(!dInode) return inodeSize;
	toSet = &dInode->inode_basic;
	// sprintf the dir inode and write back to the block
	// basic inode
	inodeSize += sprintf(buffer, "{size: %lu, uid: %d, gid:%d, mode:%d, atime:%ld, ctime:%ld, mtime:%ld, linkcount:%d, filename_to_inode_dict: {",
			toSet->size, toSet->uid, toSet->gid, toSet->mode, toSet->timeInfo.atime, toSet->timeInfo.ctime, toSet->timeInfo.mtime,
			 toSet->linkCount);
	// filename_to_inode_dir
	for(int i=0; i<toSet->linkCount; i++){
		if(dInode->dirRootEntry[i].type == file)	entryType = 'f';
		if(dInode->dirRootEntry[i].type == directory)	entryType = 'd';
		entrySize= sprintf(entry, "%c:%s:%d, ", entryType, dInode->dirRootEntry[i].fileName, dInode->dirRootEntry[i].inodeNumber);
		if(i == toSet->linkCount - 1){
			strncpy(&entry[entrySize-2], "}}", 2);
		}
		strncpy(&buffer[inodeSize], entry, entrySize);
		inodeSize += entrySize;
	}
	// write to the directory inode
	WriteNthBlock(dInode->inodeNumber, buffer, inodeSize);
	//printf("Write %d block, inode: %s", dInode->inodeNumber, buffer);
	return inodeSize;
}
// Write file inode
int WriteFileInode(FILE_INODE *fInode){
	char buffer[BLOCK_SIZE];
	int inodeSize = 0;
	INODE *toSet;
	if(!fInode)	return inodeSize;
	toSet = &fInode->inode_basic;
	inodeSize += sprintf(buffer,
			"{size: %lu, uid: %d, gid:%d, mode:%d, linkcount:%d, atime:%ld, ctime:%ld, mtime:%ld, indirect:%d location:%d}",
			toSet->size, toSet->uid, toSet->gid, toSet->mode, toSet->linkCount, toSet->timeInfo.atime, toSet->timeInfo.ctime, 
			toSet->timeInfo.mtime, fInode->indirect, fInode->location);
	WriteNthBlock(fInode->inodeNumber, buffer, inodeSize);
	return inodeSize;
}

//---------------------------------------------------------------------------------------------------------------
// Read directory INODE
int ReadDir(FS *fs, DIR_INODE *dInode){
	int valid, entryNum;
	char buffer[BLOCK_SIZE];
	dInode->inode_basic.type = directory;
	ReadNthBlock(dInode->inodeNumber, buffer);
	LoadInodeInfoFromBuffer(&dInode->inode_basic, buffer);	// load basic info to Inode
	entryNum = LoadLinkEntry(dInode, buffer);	// load link entries into Inode
	if((valid = CheckDir(fs, dInode, entryNum))){
		return -1;
	}
	// free the DIR_INODE
	//if(dInode->dirRootEntry)	free(dInode->dirRootEntry);
	//free(dInode);
	
	return 0;
}


// Read file INODE
int ReadFile(FS *fs, FILE_INODE *fInode){
	//printf("Read file block Id: %d\n", blockId);
	char buffer[BLOCK_SIZE];
	fInode->inode_basic.type= file;
	ReadNthBlock(fInode->inodeNumber, buffer);
	LoadInodeInfoFromBuffer(&fInode->inode_basic, buffer); // load basic info to Inode
	LoadFileEntry(fInode, buffer);
	fInode->locationArrayLength = LoadBlockArray(fInode->location, &(fInode->locationArray));
	CheckFile(fs, fInode);
	
	return 0;
}

// 

/*--------------------------------------------------------------------------------------*/
/* check part:
 * check Dir
 * check file, indirect & size
 * check free blocks
*/
// check whether time are in the future
// if ine future, set to current time
 int CheckTime(FS_TIME *t){
 	if(INVALID_TIME(t->atime)){
 		printf("Invalid atime: %ld\n", t->atime);
 		SET_TO_CURRENT_TIME(t->atime);
 		printf("Modify atime to current time: %ld\n", t->atime);
 		return -1;
 	}
 	if(INVALID_TIME(t->mtime)){
 		printf("Invalid mtime: %ld\n", t->mtime);
 		SET_TO_CURRENT_TIME(t->mtime);
 		printf("Modify mtime to current time: %ld\n", t->mtime);
 		return -2;
 	}
 	if(INVALID_TIME(t->ctime)){
 		printf("Invalid ctime: %ld\n", t->ctime);
 		SET_TO_CURRENT_TIME(t->ctime);
 		printf("Modify ctime to current time: %ld\n", t->ctime);
 		return -3;
 	}
 	return 0;
 }
 // linkcount has been checked when readDir
 // check whether the dir has '.' && '..'
int CheckDir(FS *fs, DIR_INODE *dInode, int entryNum){	
	//printf("%d\n",entryNum);
	int currentDirIndex, parentDirIndex, linkCount = dInode->inode_basic.linkCount;
	bool inode = true, currentDirExist = false, parentDirExist = false;
	for(int i=0; i<entryNum; i++){
		//printf("InodeIndex: %d, Inode_type:%d, Inode_fileName:%s\n", i, dInode->dirRootEntry[i].type, dInode->dirRootEntry[i].fileName);
		if(dInode->dirRootEntry[i].type == file){
			FILE_INODE *fInode = (FILE_INODE*)malloc(sizeof(FILE_INODE));
			fInode->inodeNumber = dInode->dirRootEntry[i].inodeNumber;
			strncpy(fInode->fileName, dInode->dirRootEntry[i].fileName, strlen(dInode->dirRootEntry[i].fileName));
			ReadFile(fs, fInode);	//  load and check the file
			if(fInode->locationArray)	free(fInode->locationArray);
			free(fInode);
		}
		if(dInode->dirRootEntry[i].type == directory){
			if(!strcmp(dInode->dirRootEntry[i].fileName,"..")){
				//parentDirExist = dInode->dirRootEntry[i].inodeNumber;
				parentDirExist = true;
				parentDirIndex = i;
				continue;
			}
			if(!strcmp(dInode->dirRootEntry[i].fileName,".")){
				//currentDirExist = dInode->dirRootEntry[i].inodeNumber;
				currentDirExist = true;
				currentDirIndex = i;
				continue;
			}
			// Recursively read and check child dir 
			// Set current dir inode number and parent dir inode number for check
			DIR_INODE *childDirInode = (DIR_INODE*)malloc(sizeof(DIR_INODE));
			childDirInode->inodeNumber = dInode->dirRootEntry[i].inodeNumber;
			childDirInode->parent_inodeNumber = dInode->inodeNumber;
			strncpy(childDirInode->dirName, dInode->dirRootEntry[i].fileName, strlen(dInode->dirRootEntry[i].fileName));
			ReadDir(fs, childDirInode);
			if(childDirInode->dirRootEntry)	free(childDirInode->dirRootEntry);
			free(childDirInode);
		}
	}
	printf("--------------------------------------\n");
	printf("Directory name: %s   Checking...\n", dInode->dirName);
	if(!CheckTime(&dInode->inode_basic.timeInfo)){
		printf("Time info is valid\n");
	}
	if(entryNum == linkCount){
		printf("Link count: %d match with the number of links in filename_to_inode_dict: %d\n", linkCount, entryNum);
	}
	else{
		printf("ERROR!!!!!Link count: %d DOES NOT match with the number of links in filename_to_inode_dict: %d\n", linkCount, entryNum);
		dInode->inode_basic.linkCount = entryNum;
		printf("Modify directory linkcount to: %d\n", entryNum);
	}
	if(currentDirExist){
		int currentInodeNumber = dInode->dirRootEntry[currentDirIndex].inodeNumber;
		if( currentInodeNumber != dInode->inodeNumber){
			printf("ERROR!!!!! Current Directory block number: %d should be %d\n", currentInodeNumber, dInode->inodeNumber);
			dInode->dirRootEntry[currentDirIndex].inodeNumber = dInode->inodeNumber;
			printf("Modify Current Directory block number to: %d\n", dInode->inodeNumber);
		}
		else printf("Current Directory block number: %d is correct\n", currentInodeNumber);
	}
	else{
		printf("Current Dir miss!\n");
		printf("Add a current directory entry! Will Finish this in next assignment!\n");
		/* add a dir entry for current
		*/
	}
	if(parentDirExist){
		int parentInodeNumber = dInode->dirRootEntry[parentDirIndex].inodeNumber;
		if(parentInodeNumber != dInode->parent_inodeNumber){
			printf("ERROR!!!!! Parent Directory block number: %d should be %d\n", parentInodeNumber, dInode->parent_inodeNumber);
			dInode->dirRootEntry[parentDirIndex].inodeNumber = dInode->parent_inodeNumber;
			printf("Modify Parent Directory block number to: %d\n", dInode->parent_inodeNumber);
		}
		else printf("Parent Directory block number: %d is correct\n", parentInodeNumber);
	}
	else{
		printf("Parent Dir miss\n");
		printf("Add a current directory entry! Will Finish this in next assignment!\n");
		/* add a dir entry for parent
		*/
	}
	// check dir inode is in free list or not
	inode = CheckBlockValid(fs, dInode->inodeNumber);
	if(inode) {printf("Inode_block: %d is not in the free block list\n", dInode->inodeNumber);}
	else printf("ERROR!!!!!Inode_block: %d is in the free block list\n", dInode->inodeNumber);
	WriteDirInode(dInode);
	return 0;
}


// check file 
// time is valid
// size is valid
// if indirect = 0, size < BLOCK_SIZE
// else	BLOCK_SIZE*(LENGTH-1) < size < BLOCK_SIZE*(LENGTH)
int CheckFile(FS *fs, FILE_INODE *fInode){
	printf("--------------------------------------\n");
	printf("File name: %s   Checking...\n", fInode->fileName);
	int fileSize = (int)fInode->inode_basic.size, length = fInode->locationArrayLength;
	bool inode = true, location = true, array = true;
	if(!CheckTime(&fInode->inode_basic.timeInfo)){
		printf("Time info is valid\n");
	}
	if(fInode->locationArrayLength && !fInode->indirect){
		printf("ERROE!!!!!Data Block is in CSV Format, Indirect should be 1\n");
		fInode->indirect = 1;
		printf("Modify File indirect to 1\n");
	}
	if(!fInode->indirect && (fileSize > BLOCK_SIZE || fileSize < 0)){
		printf("ERROE!!!!!File Size: %d should range from 0 to BLOCK_SIZE\n", fileSize);	return -1;
	}
	if(fInode->indirect){
		//printf("%d\n", fInode->locationArray[0]);
		if(fileSize >= length*BLOCK_SIZE){
			printf("ERROE!!!!!File Size: %d exceeds locationArraySize: %d*BLOCK_SIZE\n", fileSize, length);	return -2;
		}
		if(fileSize < (length-1)*BLOCK_SIZE){
			printf("ERROE!!!!!File Size: %d too small for locationArraySize: %d*BLOCK_SIZE\n", fileSize, length);	return -2;
		}
	}
	printf("File size: %d is valid with indirect: %d ",fileSize, fInode->indirect);
	if(fInode->indirect)	printf("with location array length: %d\n", length);
	else	printf("\n");
	inode = CheckBlockValid(fs, fInode->inodeNumber);
	if(inode) {printf("Inode_block: %d is not in free list\n", fInode->inodeNumber);}	else{printf("ERROR!!!!!Inode_block: %d is in free list\n",fInode->inodeNumber);}
	location = CheckBlockValid(fs, fInode->location);
	if(location) {printf("Location_block: %d is not in free list\n", fInode->location);}	else{printf("ERROR!!!!!Location_block: %d is in free list\n",fInode->location);}
	if(fInode->indirect){
		for(int i=0; i<fInode->locationArrayLength; i++){
			if(!CheckBlockValid(fs, fInode->locationArray[i]))	array = false;
		}
	}
	if(array) printf("All location array blocks are not in free list\n");
	WriteFileInode(fInode);
	return 0;
}

int main(){
	const char* sep = "-------------------------------------------------------";
	const char* checkSuper = "*******************Check SUPER Block*******************";
	const char* checkDF = "*****************Check Dirs and Files******************";
	const char* checkFBL = "*****************Check Free Block List*****************";
	int valid;
	FS *fs = (FS*)malloc(sizeof(FS));
	DIR_INODE *rootDir = (DIR_INODE*)malloc(sizeof(DIR_INODE));
	printf("\n%s\n%s\n%s\n\n", sep, checkSuper, sep);
	if((valid = LoadFS(fs))){	// load super block and check
		printf("FATAL ERROR, this may not be FUSE file system\n");
		//return 0;
	}
	else{
		printf("DevId is correct, file system creation time is correct\n");
	}
	fs->blockMap = (bool*)malloc(sizeof(bool)*fs->maxBlocks);
	LoadFreeBlockList(fs);
	rootDir->inodeNumber = fs->root;
	rootDir->parent_inodeNumber = fs->freeEnd;
	printf("\n%s\n%s\n%s\n\n", sep, checkDF, sep);
	/*printf("--------------------------------------\n");
	*/
	strncpy(rootDir->dirName, "root", 5);
	// root directory parent dir block number is itself
	// use recursio to check directory and files
	ReadDir(fs, rootDir);
	
	// check free block list
	printf("\n%s\n%s\n%s\n\n", sep, checkFBL, sep);
	valid = CheckAllBlocks(fs);
	if(valid)	printf("Free Block list DOES NOT leave any blocks missing\n");
	// free fs & root dir
	if(rootDir->dirRootEntry)	free(rootDir->dirRootEntry);
	free (rootDir);
	free(fs->blockMap);
	free(fs);
}