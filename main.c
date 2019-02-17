#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

typedef struct _OrderNode {
	struct _OrderNode* pNext;
	void* pValue;
} OrderNode, *POrderNode;

void PushOrder(POrderNode *pHead, void * pValue)
{
	if (*pHead) {

		POrderNode node = *pHead;
		while (node->pNext)
			node = node->pNext;
		node->pNext = (POrderNode)malloc(sizeof(OrderNode));
		if (!node->pNext){

			fprintf(stderr, "Error when trying to malloc mem for the next node!\n");
			return;

		}
		node->pNext->pValue = pValue;
		node->pNext->pNext = NULL;

	}
	else {

		*pHead = (POrderNode)malloc(sizeof(OrderNode));
		if (!(*pHead)){
			
			fprintf(stderr, "Error when trying to malloc mem for node in HEAD!\n");
			return;

		}
		(*pHead)->pValue = pValue;
		(*pHead)->pNext = NULL;

	}
	
}

void* PopOrder(POrderNode *pHead)
{

	if (!*pHead) return NULL;

	POrderNode node = *pHead;
	*pHead = (*pHead)->pNext;
	void* retVal = node->pValue;

	free(node);
	return retVal;

}

void printError(char* fullPath, char* message, char* filename)
{
	fprintf(stderr, "%s:%s:%s\n", fullPath, message, filename);
}

int SearchFile(char* rootPath, int* countFilesWatched, char* fileName)
{
	POrderNode dirsToLook = NULL;
	int fileFound = FALSE;

	DIR* dd = opendir(rootPath);
	if (!dd) return (printError(rootPath, "erroropendir", rootPath), -1);

	//Skipping . and ..
	readdir(dd);
	readdir(dd);

	for (struct dirent* entity = readdir(dd); entity && !fileFound; entity = readdir(dd))
	{

		(*countFilesWatched)++;

		char fullPath[PATH_MAX];
		sprintf(fullPath, "%s/%s", strcmp(rootPath, "/") == 0 ? "" : rootPath, entity->d_name);
		struct stat ed = {0};
		int retVal = stat(fullPath, &ed);

		if (retVal == 0){

			unsigned char dMode = ed.st_mode >> 15 & 7;

			if (!dMode) {
				// Catalog

				char* buffer = (char*)malloc(sizeof(char)*PATH_MAX);
				if (!buffer) return fprintf(stderr, "Error when trying to malloc mem for temp catalog path!\n");
				strcpy(buffer, fullPath);
				PushOrder(&dirsToLook, buffer);

			}
			else if (strcmp(entity->d_name, fileName) == 0){

				fileFound = TRUE;

				printf("File has been found! Scanned: %d files and folders\n", *countFilesWatched);
				
				unsigned char uMode = ed.st_mode >> 6 & 7;
				unsigned char gMode = ed.st_mode >> 3 & 7;
				unsigned char aMode = ed.st_mode >> 0 & 7;

				char formatedTime[256];
				struct tm *tmp;
				tmp = localtime(&ed.st_ctime);
				if (tmp == NULL) {

					fprintf(stderr, "Error on converting to localtime...");
					continue;

	            }
	            if (strftime(formatedTime, sizeof(formatedTime), "%H:%M:%S %d/%m/%Y", tmp) == 0) {

					fprintf(stderr, "strftime returned 0");
					continue;

				}


				printf(
					"Full Path: %s\nSize: %ld bytes\nCreated: %s\nMode: %d%d%d\nIndex descriptor number: %ld\n",
					fullPath,
					ed.st_size,
					formatedTime,
					uMode, gMode, aMode,
					ed.st_ino
				);

			}

		}
		else {

			printError(fullPath, "errorgetstat", entity->d_name);

		}

	}

	// Checking subfolders
	while(dirsToLook && !fileFound)
	{

		char* fullPath = (char*)PopOrder(&dirsToLook);
		int retVal = SearchFile(fullPath, countFilesWatched, fileName);
		free(fullPath);
		if (retVal == 0) fileFound = TRUE;
	}


	// Cleaning folders if we have them
	while (dirsToLook)
	{

		free(PopOrder(&dirsToLook));

	}

	return fileFound ? 0 : 1;
}

int main(int argc, char* argv[], char* envp[])
{

	if (argc != 3) return fprintf(stderr, "Invalid arguments count! Should be 2 arguments!\n");

	int countFilesWatched = 0;

	char path[PATH_MAX];
	sprintf(path, "%s", argv[1]);

	int retVal = SearchFile(path, &countFilesWatched, argv[2]);

	if (retVal == 1) {

		printf("File has NOT been found!\n");

	}
	else if (retVal != 0){

		fprintf(stderr, "Fatal error occured while trying to search file!\n");

	}

	return 0;
}