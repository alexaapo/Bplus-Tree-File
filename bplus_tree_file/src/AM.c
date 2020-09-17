#include "AM.h"
#include "help.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//Globar variable to keep the last error that happened.
int AM_errno = AME_OK;

#define CALL_BF(call)          \
{                             \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      AM_errno = AME_BF_ERROR;\
      return AM_errno;    \
    }                         \
}

//Initialization of Global Arrays...
void AM_Init() 
{
  BF_Init(MRU);
  for(int i=0;i<MAX_OPEN_FILES;i++) //MAX_OPEN_FILES = MAX_SCAN_FILES = 20
  {
    //For OpenFile...
    OpenFile[i].flag=0;
    OpenFile[i].root=0;
    OpenFile[i].filedesc=0;
    OpenFile[i].openscans=0;
    OpenFile[i].max_entries=0;
    OpenFile[i].attributes.attrType1 = '0';
    OpenFile[i].attributes.attrLength1 = 0;
    OpenFile[i].attributes.attrType2 = '0';
    OpenFile[i].attributes.attrLength2 = 0;
    strcpy(OpenFile[i].filename,"0");
    
    //For OpenScan...
    OpenScan[i].flag=0;
    OpenScan[i].indexdesc=0;
    OpenScan[i].start_block=0;
    OpenScan[i].record=0;
    OpenScan[i].op=0;
    OpenScan[i].value=NULL;
  }
}

int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2) 
{
  if(access(fileName, F_OK)!=-1)
  {
    AM_errno = AME_FILE_ALREADY_EXISTS;
    return AM_errno;
  }
  if((attrType1 == 'i' || attrType1 == 'f') && (attrLength1 !=4 ))	
  {
    AM_errno = AME_TYPE_LENGTH_NOT_MATCH;
    return AM_errno;
  }
  else if (attrType1=='c' &&(( attrLength1>255)||(attrLength1<1)))
  {
    AM_errno = AME_TYPE_LENGTH_NOT_MATCH;
    return AM_errno;
  }
  if((attrType2=='i' || attrType2=='f')&&(attrLength2!=4))
  {
    AM_errno = AME_TYPE_LENGTH_NOT_MATCH;
    return AM_errno;
  }
  else if (attrType2=='c' &&(( attrLength2>255)||(attrLength2<1)))
  {
    AM_errno = AME_TYPE_LENGTH_NOT_MATCH;
    return AM_errno;
  } 
  //Create a file with name filename.
  CALL_BF(BF_CreateFile(fileName));

  //Open the file with name filename.
  int file_desc, zero=0;
  CALL_BF(BF_OpenFile(fileName,&file_desc));

  //I will write to the first block of the file to make it an AM_FILE.
  BF_Block* block;
  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(file_desc,block)); //Allocate memory for the first block I created.
  
  char* data;
  data=BF_Block_GetData(block); //Take the data of the first block.
  int integer=200;
  memcpy(data,&integer,sizeof(int)); //I choose a random intenger to show that this is an AM_FILE.
  //Store useful information (metada)
  memcpy(data+sizeof(int),&attrType1,sizeof(char));
  memcpy(data+sizeof(int)+sizeof(char),&attrLength1,sizeof(int));
  memcpy(data+2*sizeof(int)+sizeof(char),&attrType2,sizeof(char));
  memcpy(data+2*sizeof(int)+2*sizeof(char),&attrLength2,sizeof(int));
  
  BF_Block_SetDirty(block); //Because we want to change the data of the first block.
  CALL_BF(BF_UnpinBlock(block));   //I unpin the block because I don't want it anymore.
  
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(file_desc));
  
  return AME_OK;
}

int AM_DestroyIndex(char *fileName) 
{
  for(int i=0;i<MAX_OPEN_FILES;i++)
  {
    if(!strcmp(OpenFile[i].filename,fileName))
    {
      AM_errno = AME_DESTROY_WITH_OPEN_FILES;
      return AME_DESTROY_WITH_OPEN_FILES;
    }
    if(OpenFile[i].openscans > 0)
    {
      AM_errno = AME_DESTROY_WITH_OPEN_SCAN_FILES;
      return AME_DESTROY_WITH_OPEN_SCAN_FILES;
    }
  }
  remove(fileName);
  return AME_OK;
}

//This function returns the position into the appropriate array, which is available
int Position_In_Array(char c)
{
  for(int i=0;i<MAX_OPEN_FILES;i++)
  {
    if(c=='F')
      if(OpenFile[i].flag==0)
        return i;  
    if(c=='S')
      if(OpenScan[i].flag==0)
        return i;
  }
  return -1; //Case that we don't have any space at all. Array is full.
}

int AM_OpenIndex (char *fileName) 
{
  int position,blocks_num;
  
  position = Position_In_Array('F');
  if (position == -1)
  {
    AM_errno = AME_NO_SPACE_IN_FILES_ARRAY;
    return AM_errno;
  }
  
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_BF(BF_OpenFile(fileName, &(OpenFile[position].filedesc)));
  CALL_BF(BF_GetBlockCounter(OpenFile[position].filedesc,&blocks_num));
  if(blocks_num==0)
  {
    AM_errno = UNCOMPATIBLE_TYPE_OF_FILE;
    return AM_errno;
  }
  CALL_BF(BF_GetBlock(OpenFile[position].filedesc, 0, block));
  
  char attrType1,  attrType2;
  int attrLength1,attrLength2;
  char* data;
  int integer;
  data=BF_Block_GetData(block);
  memcpy(&integer,data,sizeof(int));
  if (integer != 200) return AME_ERROR;

  memcpy(&attrType1, data+sizeof(int), sizeof(char));
  memcpy(&attrLength1, data+sizeof(int)+sizeof(char), sizeof(int));
  memcpy(&attrType2, data+2*sizeof(int)+sizeof(char), sizeof(char));
  memcpy(&attrLength2, data+2*sizeof(int)+2*sizeof(char), sizeof(int));  

  //Store attribute values in array of Open Files.
  OpenFile[position].attributes.attrType1 = attrType1;
  OpenFile[position].attributes.attrLength1 = attrLength1;
  OpenFile[position].attributes.attrType2 = attrType2;
  OpenFile[position].attributes.attrLength2 = attrLength2;
  OpenFile[position].flag=1;
  OpenFile[position].max_entries = Entries_in_dataBlock(attrLength1, attrLength2);
  OpenFile[position].max_pairs = Pairs_in_indexBlock(attrLength1);
  strcpy(OpenFile[position].filename, fileName);
  
  CALL_BF(BF_UnpinBlock(block));  
  BF_Block_Destroy(&block);
  
  return position;
}

int AM_CloseIndex (int indexDesc) 
{
  if(OpenFile[indexDesc].openscans > 0)
  {
    AM_errno = AME_OPEN_SCAN_FOR_THIS_FILE;
    return AM_errno;
  }
	
	BF_CloseFile(OpenFile[indexDesc].filedesc);					 
	OpenFile[indexDesc].flag = 0;
  OpenFile[indexDesc].root = 0;
  OpenFile[indexDesc].filedesc = 0;
  OpenFile[indexDesc].openscans = 0;	
  OpenFile[indexDesc].max_entries = 0;	
  OpenFile[indexDesc].attributes.attrType1 = '0';
  OpenFile[indexDesc].attributes.attrLength1 = 0;
  OpenFile[indexDesc].attributes.attrType2 = '0';
  OpenFile[indexDesc].attributes.attrLength2 = 0;
  strcpy(OpenFile[indexDesc].filename,"0");

  return AME_OK;
}

int AM_InsertEntry(int indexDesc, void *value1, void *value2) 
{
  int critical_block,blocks_num=0,one=1,two=2;
  BF_Block* datablock;
  BF_Block* indexblock;
  void* critical_key=NULL;
  void* temp_critical_key=NULL;
  char* data;
  char* indexdata;
  int size1 = OpenFile[indexDesc].attributes.attrLength1;
  int size2 = OpenFile[indexDesc].attributes.attrLength2;
  int father,entries_num,pairs_num,temp_blocks_num;

  if (OpenFile[indexDesc].flag == 0)
	{
		AM_errno = AME_FILE_NOT_FOUND;
		return AM_errno;	
	}
  int root = OpenFile[indexDesc].root;

  //Case: Until B+ tree obtains a root.
  if(root == 0)
  {
    CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc, &blocks_num));
    //Case: We have only one block(metadata).
    if(blocks_num == 1)
    { 
      CALL_BF(Create_Block(&datablock,'l',indexDesc));
      data = BF_Block_GetData(datablock);
      //Store value1,value2 into first leaf node.
      memcpy(data+sizeof(char)+2*sizeof(int),value1,size1);
      memcpy(data+sizeof(char)+2*sizeof(int)+size1,value2,size2);
      BF_Block_SetDirty(datablock); //aytoi den to exoyn ayto
      CALL_BF(BF_UnpinBlock(datablock));
      BF_Block_Destroy(&datablock);
    }
    //Case: We have one leaf node allocated.
    else if(blocks_num == 2)
    {
      int fd = OpenFile[indexDesc].filedesc;
      int max_entries = OpenFile[indexDesc].max_entries;
      int entries_counter;

      BF_Block_Init(&datablock);
      CALL_BF(BF_GetBlock(fd,1,datablock));
      data = BF_Block_GetData(datablock);
      memcpy(&entries_counter,data+sizeof(char),sizeof(int));
      
      //Case: We have space yet into the first leaf node.
      if(entries_counter < max_entries)
      {
        Sort_Entries_In_Block(data,value1,value2,indexDesc);
      }
      //Case: We have no space into first lead node, so we have to allocate a 2nd leaf node.
      else
      {
        critical_key = malloc(size1);
        Break_Block(data,value1,value2,&critical_key,indexDesc);	//break the first data block in two and split the entries
				Create_Block(&indexblock,'i',indexDesc);					//create a root block
				indexdata = BF_Block_GetData(indexblock);							
			
        memcpy(indexdata+sizeof(char)+2*sizeof(int),critical_key,size1); 
				memcpy(indexdata+sizeof(char)+sizeof(int),&one,sizeof(int));				//the old block is block number 1
				memcpy(indexdata+sizeof(char)+2*sizeof(int)+size1,&two,sizeof(int));	//the newly created block is the block number 2
				
				OpenFile[indexDesc].root = 3; 										//the new root is block number 3
        free(critical_key);
        BF_UnpinBlock(indexblock);
				BF_Block_Destroy(&indexblock);				

			}
			BF_Block_SetDirty(datablock);
			BF_UnpinBlock(datablock);
			BF_Block_Destroy(&datablock);
    }
  }
  //We have a root (indexblock) and root has 2 children (datablocks)
  else
  {
    stack_node* stack = NULL;
    CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc, &blocks_num));
    critical_block = Search_Block(value1,&stack,indexDesc);				//Get the block where the key should be inserted
		if(critical_block == -1)											//error there is no such block
		{
			AM_errno = AME_BLOCK_NOT_FOUND;
			return AME_BLOCK_NOT_FOUND;
		}

		if (critical_block!=-1)											//If the block was found without errors
		{
			BF_Block_Init(&datablock);
			BF_Block_Init(&indexblock);
			CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,critical_block,datablock));
			data = BF_Block_GetData(datablock);
			memcpy(&entries_num,data+sizeof(char),sizeof(int));
			if (entries_num < OpenFile[indexDesc].max_entries)							//If we have enough space in the data block that was found
			{
				Sort_Entries_In_Block(data,value1,value2,indexDesc);					//find where in the data block the new record should be
				
				BF_Block_SetDirty(datablock);												
				CALL_BF(BF_UnpinBlock(datablock));											
				BF_Block_Destroy(&datablock);																				
			}
			else															//There is not enough space in the data block,we need to break it in two
			{
				critical_key = malloc (size1);
				temp_critical_key = malloc (size1);
				Break_Block(data,value1,value2,&critical_key,indexDesc);	//Break the block and get the key that should go to the index levels
				BF_Block_SetDirty(datablock);
				CALL_BF(BF_UnpinBlock(datablock));
				BF_Block_Destroy(&datablock);
				
				int flag = 1;
				while (((stack)!=NULL)&&(flag))								//Go up the index levels until we reach the root or find an index block that has space
				{
					father = pop(&stack);									//Get the father index block
					CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,father,indexblock));
					data = BF_Block_GetData(indexblock);
					memcpy(&pairs_num,data+sizeof(char),sizeof(int));
					if (pairs_num < OpenFile[indexDesc].max_pairs)						//if the key fits in the index block just insert it 
					{
						CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc,&blocks_num));
						blocks_num = blocks_num -1;
						Sort_Entries_In_Block(data,critical_key,&blocks_num,indexDesc);//Sort the index block with the new key 
						flag = 0;											//No need to continue going up the key's place was found
					}
					else 													//The father does not have space
					{
						CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc,&blocks_num));
						blocks_num = blocks_num -1;
						Break_Block(data,critical_key,&blocks_num,&temp_critical_key,indexDesc);//break the father and get the key that goes up
						memcpy(critical_key,temp_critical_key,size1);

					}
					BF_Block_SetDirty(indexblock);
					BF_UnpinBlock(indexblock);
				}
				if(flag)													//We reached the root block
				{
					CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc,&blocks_num));
					BF_Block_Destroy(&indexblock);
					CALL_BF(Create_Block(&indexblock,'i',indexDesc));				//Create a new root block
					data = BF_Block_GetData(indexblock);
					data = data + (sizeof(char) + sizeof(int));
					memcpy(data,&(OpenFile[indexDesc].root),sizeof(int));			//The left pointer is the previous root
					memcpy(data+sizeof(int),critical_key,size1);		//Write down the key 
					temp_blocks_num = blocks_num-1;
					memcpy(data+sizeof(int)+size1, &temp_blocks_num,sizeof(int));////The right pointer is the last index block that has been created before the root
					OpenFile[indexDesc].root=blocks_num;  //Update root.
					BF_UnpinBlock(indexblock);

				}
				free(critical_key);
				free(temp_critical_key);
			}
			BF_Block_Destroy(&indexblock);
		}
		delete_stack(stack);												//free the stack
  }
  	return AME_OK;     
}

int AM_OpenIndexScan(int indexDesc, int op, void *value) 
{
  int scanDesc,block_num, next_block, First_Data_Block, entries, count;
  void* entry_key;
  BF_Block* block;
  char* data;
  BF_Block_Init(&block);
  int fd = OpenFile[indexDesc].filedesc;
  int size1 = OpenFile[indexDesc].attributes.attrLength1;
  int size2 = OpenFile[indexDesc].attributes.attrLength2;
  int type1 = OpenFile[indexDesc].attributes.attrType1;
  entry_key = malloc(size1);

  if(OpenFile[indexDesc].flag == 0)											//The file specified is not open in the open_files array
  {
    AM_errno = AME_FILE_NOT_FOUND;
    return AM_errno;
  }

  scanDesc = Position_In_Array('S');
  if(scanDesc == -1)									//There is no space in the open_scans array
  {
    AM_errno = AME_NO_SCAN_SPACE;
    return AM_errno;				
  }

  OpenScan[scanDesc].indexdesc = indexDesc;
  OpenScan[scanDesc].flag = 1;
  OpenScan[scanDesc].op = op;
  OpenScan[scanDesc].value = value;
  OpenFile[indexDesc].openscans++;

  First_Data_Block = first_data_block(indexDesc);
  next_block = First_Data_Block;

  while(next_block != 0)
  {
    count = 0;
    CALL_BF(BF_GetBlock(fd, next_block, block));
    data = BF_Block_GetData(block);
    memcpy(&entries,data+sizeof(char),sizeof(int));
    data = data + (sizeof(char)+2*sizeof(int));

    while(count < entries)
    {
      memcpy(entry_key,data+count*(size1+size2),size1);
      if(OS_Compare(entry_key, value, op, type1))
      {
        OpenScan[scanDesc].start_block = next_block;
        OpenScan[scanDesc].record = count;
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        free(entry_key);
        return scanDesc;
      }
      count++;
    }
    data = BF_Block_GetData(block);
    CALL_BF(BF_UnpinBlock(block));
    memcpy(&next_block,data+sizeof(char)+sizeof(int),sizeof(int));

  }
  BF_Block_Destroy(&block);

  free(entry_key);
  return scanDesc;
}

void* AM_FindNextEntry(int scanDesc) 
{
  int i,entries,size1,size2,next;
  BF_Block *block;
  char *data,type1;
  void *key,*info;

  if( OpenScan[scanDesc].flag == 0) 
  {
      AM_errno = AME_SCAN_NOT_FOUND;
      return NULL;
  }
  int indexdesc = OpenScan[scanDesc].indexdesc;
  int fd =OpenFile[indexdesc].filedesc;
  size1 = OpenFile[indexdesc].attributes.attrLength1;
  size2 = OpenFile[indexdesc].attributes.attrLength2;
  type1 = OpenFile[indexdesc].attributes.attrType1;
  key=malloc(size1);
  info=malloc(size2);
  BF_Block_Init(&block);

  while(OpenScan[scanDesc].start_block != 0)                       	//Search blocks from "start block" 
  {
    if(BF_GetBlock(OpenFile[indexdesc].filedesc,OpenScan[scanDesc].start_block,block) != BF_OK)     //get the info of each block
    {
      AM_errno = AME_BF_ERROR;
      return NULL;
    }
    data = BF_Block_GetData(block);
    data = data + sizeof(char);
    memcpy(&entries,data,sizeof(int));
    data = data + sizeof(int);
    memcpy(&next,data,sizeof(int));
    
    data = data + sizeof(int) + OpenScan[scanDesc].record*(size1+size2);

    while(OpenScan[scanDesc].record < entries)              		 //for every entry in the block
    {
      memcpy(key,data,size1);
      if(OS_Compare(key,OpenScan[scanDesc].value,OpenScan[scanDesc].op,type1))    		 //then check if the OP is true
      {
        (OpenScan[scanDesc].record) = (OpenScan[scanDesc].record) + 1;       
        memcpy(info,data+size1,size2);                              //if it is true return the entry
        return info;
      }
      else
      {
          (OpenScan[scanDesc].record) = (OpenScan[scanDesc].record) + 1;       
      }
      data = data + (size1+size2);
    }
    
    if(BF_UnpinBlock(block)!=BF_OK)                   						 //go to next block when there are no more entries
    {
        AM_errno = AME_BF_ERROR;
        return NULL;
    }
    OpenScan[scanDesc].record = 0;
    OpenScan[scanDesc].start_block = next;
  }    
  AM_errno = AME_EOF;
  return NULL;
}

int AM_CloseIndexScan(int scanDesc) 
{
  int pos_in_openfile;
	if(OpenScan[scanDesc].flag != 1)
  {
    AM_errno = AME_SCAN_NOT_FOUND;
    return AM_errno;
  }
  pos_in_openfile = OpenScan[scanDesc].indexdesc;
  OpenFile[pos_in_openfile].openscans--;
  OpenScan[scanDesc].flag=0;
  OpenScan[scanDesc].indexdesc=0;
  OpenScan[scanDesc].start_block=0;
  OpenScan[scanDesc].record=0;
  OpenScan[scanDesc].op=0;
  OpenScan[scanDesc].value=NULL;

  return AME_OK;
}

void AM_PrintError(char *errString) 
{
  if(errString!=NULL)
    printf("%s", errString);
    
  switch(AM_errno)
  {
    case AME_ERROR:
      printf("Error!\n");
      break;
    
    case AME_BF_ERROR:
      printf("BF Error!\n");
      break;

    case AME_FILE_NOT_FOUND:
      printf("Didn't find this file!\n");
      break;

    case AME_FILE_ALREADY_EXISTS:
      printf("This file already exists!\n");
      break;

    case AME_TYPE_LENGTH_NOT_MATCH:
      printf("\nThe type and the length given do not match\n");
      break;

    case AME_DESTROY_WITH_OPEN_FILES:
      printf("Try to destroy when some files hadn't been closed.\n");
      break;

    case AME_DESTROY_WITH_OPEN_SCAN_FILES:
      printf("Try to destroy when some files had open scanings.\n");
      break;
    
    case AME_WRONG_FILE_TYPE:
      printf("This file isn't an AM file!\n");
      break;
    
    case AME_OPEN_SCAN_FOR_THIS_FILE:
      printf("Attempt to close a file when it was scanning!\n");
      break;
    
    case AME_EOF:
      printf("End of file has been reached!\n");
      break;

    case AME_NO_SPACE_IN_FILES_ARRAY:
      printf("There is no space in Open Files Array!\n");
      break;
    
    case AME_SCAN_NOT_FOUND:
      printf("This scan number doesn't exist!\n");
      break;
    
    case AME_BLOCK_NOT_FOUND:
      printf("Couldn't find this block!\n");
      break;
    
    case AME_NO_SCAN_SPACE:
      printf("There is no space in Open Scan Array!\n");
      break;
    
    case AME_SCAN_VALUE_NOT_EXISTS:
      printf("Didn't find any entry at all with the condition that you gave!\n");
      break;

    case UNCOMPATIBLE_TYPE_OF_FILE:
      printf("This isn't an AM file...\n");
      break;
    
    default:
      printf("Uknown Error!\n");
      break;
  }
}

void AM_Close() 
{
  BF_Close();
}

/////////////////////////////////////////////////
////////FUNCTIONS JUST FOR DEBUGGING/////////////
/////////////////////////////////////////////////

int tsoytsos(int indexDesc,int block)
{    
  BF_Block * ROOOT;
  BF_Block_Init(&ROOOT);

  CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,block,ROOOT));
  char* data2 = BF_Block_GetData(ROOOT);
  int temp;
  memcpy(&temp,data2+sizeof(char)+sizeof(int),sizeof(int));
  print_data_block(data2,indexDesc);
  printf("********************************** next->%d\n",temp);
  
  CALL_BF(BF_UnpinBlock(ROOOT));
  BF_Block_Destroy(&ROOOT);
  return temp;
}

int indextsoutsos(int indexDesc)
{
  int root = OpenFile[indexDesc].root;
  if(root!=0)
  {
    BF_Block * ROOOT;
    BF_Block_Init(&ROOOT);
    printf("indextsoutsos %d\n\n",root);
    CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,root,ROOOT));
    char* data1 = BF_Block_GetData(ROOOT);
    print_index_block(data1,indexDesc);
    printf("**********************************\n");
    CALL_BF(BF_UnpinBlock(ROOOT));
    BF_Block_Destroy(&ROOOT);
  }
}

