#include "help.h"
#include "AM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
///////////////////////////////////////////////////////////////////
////////////////////////Stack implementation//////////////////////
/////////////////////////////////////////////////////////////////
int push(stack_node** stack , int num)
{
	stack_node* temp =  malloc(sizeof(stack_node));
	temp->data = num;
	temp->next = *(stack);
	*stack = temp;
	return 0;
}

int pop(stack_node** stack)
{
	stack_node* temp;
	temp = (*stack)->next;
	int retval = (*stack)->data;
	free(*stack);
	*stack = temp;
	return retval;
}

void delete_stack(stack_node* stack)
{
	stack_node* temp;
	while (stack!=NULL)
	{
		temp = stack;
		stack = stack->next;
		free(temp);
	}
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////FUNCTIONS WHICH HELP US//////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//Finds how many entries can a datablock hold.
int Entries_in_dataBlock(int attrLength1, int attrLength2)
{
    int nine_bytes = 2*sizeof(int) + sizeof(char); //1 byte for l, 4 bytes for counter, 4 bytes for next block.
    int freespace = BF_BLOCK_SIZE - nine_bytes;
    int entries_bytes = attrLength1 + attrLength2;
    int max_entries = freespace/entries_bytes;
    return max_entries;
}

//Finds how many keys-pairs can an indexblock hold.
int Pairs_in_indexBlock(int attrLength1)
{
    int max_pairs = (BF_BLOCK_SIZE-sizeof(char)-2*sizeof(int))/(attrLength1+sizeof(int));

    return max_pairs;
}

//This function creates a new block, either index or data block.
int Create_Block(BF_Block **block, char descriptory, int indexDesc)
{
	char* data;
	int zero=0,max,one=1;

	BF_Block_Init(block);
	CALL_BF(BF_AllocateBlock(OpenFile[indexDesc].filedesc,*block));
	data = BF_Block_GetData(*block);
	memcpy(data,&descriptory,sizeof(char));
	memcpy(data+sizeof(char),&one,sizeof(int));  //Counter=1, because we are going to insert a pair/entry.
	
	data = data + (sizeof(char)+sizeof(int));
    //Case: This is an index block, so eliminate all pointers.
	if (descriptory == 'i')	
	{
		for ( int i = 0; i < OpenFile[indexDesc].max_pairs; i++)
		{
			memcpy(data,&zero,sizeof(int));
			data = data + (OpenFile[indexDesc].attributes.attrLength1);  
		}
	}
	else if(descriptory == 'l')
		memcpy(data,&zero,sizeof(int)); //Next Block

	BF_Block_SetDirty(*block);

	return AME_OK;
}

//This function sorts entries or keys (depends), cause we have inserted one new entry or key.
void Sort_Entries_In_Block(char* data, void* key, void* value, int indexDesc)
{
	char* entries_to_move;
	char descriptory;
	int pos=0,blocks_num,entries,total_entries;
	int size1 = OpenFile[indexDesc].attributes.attrLength1;
	int size2 = OpenFile[indexDesc].attributes.attrLength2;
    int size = size1;

	memcpy(&descriptory,data,sizeof(char));
	//Leaf_Node
    if (descriptory == 'l') 
    {    
		size = size + size2;		
	}
	//Index Node
    else if (descriptory == 'i')
    {
	    size = size + sizeof(int);									
	}
	pos = Pos_Of_Entry_In_Block(data,key,indexDesc);            //Find where in the block the entry should be inserted
	
	memcpy(&entries,data+sizeof(char),sizeof(int));     //How many entries are in the block
	total_entries = entries + 1;
	memcpy(data+sizeof(char),&total_entries,sizeof(int));   //Update num of entries
	
    //Case: Our entry has to be inserted in the middle, not last entry.
	if((entries-pos)!=0)										
	{
		entries_to_move = malloc((entries-pos)*size);   //Allocate memory to move entries which are at the right of key								
		memcpy(entries_to_move,data+sizeof(char)+2*sizeof(int)+pos*size,(entries-pos)*size); //Copy them.
	}
	data = data + (sizeof(char)+2*sizeof(int)+pos*size);  //Insert the new Entry in the right place

	memcpy(data,key,size1);
	data = data + size1;

    //Leaf_Node
	if (descriptory == 'l')
	{
		memcpy(data,value,size2);
		data = data + size2;
	}
    //Index_Node
	else if (descriptory == 'i')
	{
		memcpy(data,value,sizeof(int));
		data =  data + sizeof(int);
	}

	if((entries-pos)!=0)										
	{
		memcpy(data,entries_to_move,(entries-pos)*size);    //Paste the Entries that should be now at the right of our new entry.
		free(entries_to_move);
	}	
}

//Finds the position of the new entry or new key inside a datablock or indexblock respectively. 
int Pos_Of_Entry_In_Block(char* data, void* key, int indexDesc)
{
    int step=0,entries,pos = 0;
	char descriptory;
	void* curr_key;

    int size1 = OpenFile[indexDesc].attributes.attrLength1;
    int size2 = OpenFile[indexDesc].attributes.attrLength2;

	curr_key = malloc(size1);
	
	memcpy(&entries,data+sizeof(char),sizeof(int));						
    memcpy(&descriptory,data,sizeof(char));
    
	if (descriptory == 'i')
	{	
        step = size1 + sizeof(int);	//How much do we need to move the pointer to get the next entry
	}
	else if (descriptory == 'l') 
    {
	    step = size1 + size2;
	}

    data = data + (sizeof(char)+2*sizeof(int));
	memcpy(curr_key,data,size1);
	while((key_compare(key,curr_key,OpenFile[indexDesc].attributes.attrType1) > 0  ) && (pos < entries))
	{
		data = data + step;
		memcpy(curr_key,data,size1);
		pos = pos + 1;
	}	
	free(curr_key);
	return pos;
}

//Compares a keys and return the appropriate value, so as to understand what desicion to take...
int key_compare(void* key1,void* key2 , char key_type)
{
	if(key_type=='c') return strcmp((char*)key1,(char*)key2);
	else if (key_type=='i')
	{
		if(*(int*)key1 > *(int*)key2) return 1;
		else if (*(int*)key1 == *(int*)key2) return 0;
		else return -1;
	}
	else 
	{
		if(*(float*)key1 > *(float*)key2) return 1;
		else if (*(float*)key1 == *(float*)key2) return 0;
		else return -1;
	}
}

//This function breaks a block (index or data) into to two blocks (by one new allocation of block) and store to variable critical_key the key that should go up.
int Break_Block(char* data,void* key,void* value, void** critical_key,int indexDesc)
{
	int blocks_num, next,Position_Of_Entry,entries_num,break_point;
	int Entries_in_1st, Entries_in_2nd; 
    char descriptory;
	void* cr_key;
	char* data1 = NULL,*data2=NULL;
	BF_Block *block;
	int size1 = OpenFile[indexDesc].attributes.attrLength1;
	int size2 = OpenFile[indexDesc].attributes.attrLength2;
	int total_size = size1, max;

	memcpy(&descriptory,data,sizeof(char));
	if(descriptory == 'l') 
	{
		total_size = total_size + size2;	//For data block this is the tota_size of key-entry
		max = OpenFile[indexDesc].max_entries;
	}
	else if (descriptory == 'i')
	{ 
		total_size = total_size + sizeof(int);	//For index block this is the total_size of key-pointer
		max = OpenFile[indexDesc].max_pairs;
	}

	CALL_BF(BF_GetBlockCounter(OpenFile[indexDesc].filedesc,&blocks_num));
	CALL_BF(Create_Block(&block,descriptory,indexDesc));
	data1 = BF_Block_GetData(block);
	
	if (descriptory == 'l')		//If the block is a data block set the pointers to the next block  
	{
		memcpy(&next,data+sizeof(char)+sizeof(int),sizeof(int));
		memcpy(data+sizeof(char)+sizeof(int),&blocks_num,sizeof(int));	//The next of the old block is the new block
		memcpy(data1+sizeof(char)+sizeof(int),&next,sizeof(int));	//The next of the new block is the old next of the old block
	}
	
	Position_Of_Entry = Pos_Of_Entry_In_Block(data,key,indexDesc);				//Find where in the leaf block the new record should be
	if(Position_Of_Entry > (max/2))											//New entry goes in the new block
	{
		Entries_in_1st = (max/2) + max%2;									//How many recs stay in the first block
		
		if(descriptory == 'l')																
		{
			Entries_in_1st = Same_Keys(data,Entries_in_1st,indexDesc);
			if(Entries_in_1st == 0)			//If the whole block has the same values 
			{
				if(key_compare(key,data+sizeof(char)+2*sizeof(int),OpenFile[indexDesc].attributes.attrType1)<0) //If the key to be inserted is less than the keys move all the keys to the new block
					Entries_in_1st = 0;
				else Entries_in_1st = max;										//Else keep all the keys and move the key alone to the new block
			}
		}
		
		memcpy(data+sizeof(char),&Entries_in_1st,sizeof(int));					//Change the number of entries in the data
		Entries_in_2nd = (max-Entries_in_1st);									//How many entries to move
		memcpy(data1+sizeof(char),&Entries_in_2nd,sizeof(int));
		memcpy(data1+sizeof(char)+2*sizeof(int),data+Entries_in_1st*total_size+sizeof(char)+2*sizeof(int),total_size*Entries_in_2nd);
		
		Sort_Entries_In_Block(data1,key,value,indexDesc);									//Now sort the newly created block
	}
	
	else if (Position_Of_Entry < (max/2))										
	{
		Entries_in_1st = (max/2);
		if(max % 2 == 0)  Entries_in_1st = Entries_in_1st -1;
		if(descriptory == 'l')
		{
			Entries_in_1st = Same_Keys(data,Entries_in_1st,indexDesc);
			if(Entries_in_1st == 0)
			{
				if(key_compare(key,data+sizeof(char)+2*sizeof(int),OpenFile[indexDesc].attributes.attrType1)<0)  Entries_in_1st=0;
				else Entries_in_1st = max;
			}
		}
		memcpy(data+sizeof(char),&Entries_in_1st,sizeof(int));					
		Entries_in_2nd = (max - Entries_in_1st);									
		memcpy(data1+sizeof(char),&Entries_in_2nd,sizeof(int));
		memcpy(data1+sizeof(char)+2*sizeof(int),data+Entries_in_1st*total_size+sizeof(char)+2*sizeof(int),total_size*Entries_in_2nd);
		
		if(descriptory == 'l' && key_compare(data1+sizeof(char)+2*sizeof(int),key,OpenFile[indexDesc].attributes.attrType1) == 0) //If the new key is equal to the keys on the right we must insert it to the right 
			Sort_Entries_In_Block(data1,key,value,indexDesc);
		else Sort_Entries_In_Block(data,key,value,indexDesc);							//Else it can go to the left to keep the balance
	}
	else                                                                   
	{
		Entries_in_1st = max/2;											
		if(descriptory == 'l')
		{
			Entries_in_1st = Same_Keys(data,Entries_in_1st,indexDesc);
			if(Entries_in_1st == 0)
			{
				if(key_compare(key,data+sizeof(char)+2*sizeof(int),OpenFile[indexDesc].attributes.attrType1)<0)  Entries_in_1st=0;
				else Entries_in_1st = max;
			}
		}
		memcpy(data+sizeof(char),&Entries_in_1st,sizeof(int));					
		Entries_in_2nd = max-Entries_in_1st;					
		memcpy(data1+sizeof(char),&Entries_in_2nd,sizeof(int));
		memcpy(data1+sizeof(char)+2*sizeof(int),data+Entries_in_1st*total_size+sizeof(char)+2*sizeof(int),total_size*Entries_in_2nd);
		if(max % 2 ==0)
		{
			if(key_compare(key,data+sizeof(char)+2*sizeof(int),OpenFile[indexDesc].attributes.attrType1)!= 0)  Sort_Entries_In_Block(data1,key,value,indexDesc);
			else Sort_Entries_In_Block(data,key,value,indexDesc);
		}
		else
		{
			if(key_compare(key,data1+sizeof(char)+2*sizeof(int),OpenFile[indexDesc].attributes.attrType1)!= 0)  Sort_Entries_In_Block(data,key,value,indexDesc);
			else Sort_Entries_In_Block(data1,key,value,indexDesc);
		}
	}
	
	cr_key = malloc(size1);
	memcpy(cr_key,data1+sizeof(char)+2*sizeof(int),size1);				//Key that needs to go up is the leftmost key of the newly created block
	
	if(descriptory == 'i')															//If we are at an index block we have to delete the first key because it went up
	{
		memcpy(&entries_num,data1+sizeof(char),sizeof(int));
		entries_num = entries_num -1;
		memcpy(data1+sizeof(char),&entries_num,sizeof(int));
		data2 = data1+sizeof(char)+2*sizeof(int)+size1;
		memcpy(data1+sizeof(char)+sizeof(int),data2,BF_BLOCK_SIZE-(sizeof(char)+2*sizeof(int)+size1));
	}	

	BF_Block_SetDirty(block);
	CALL_BF(BF_UnpinBlock(block));
	BF_Block_Destroy(&block);

	memcpy((*critical_key),cr_key,size1);
	free(cr_key);
}

//This function calculates the position that we have to break the block,due to same_keys.
int Same_Keys(char* data,int break_point, int indexDesc)
{
	void* key1,*key2;
	char* data1 = data;
	int descriptory,entries_counter,counter=break_point-1;
	int temp_break_point=break_point;
	int size1 = OpenFile[indexDesc].attributes.attrLength1;
	int size2 = OpenFile[indexDesc].attributes.attrLength2;
	
	memcpy(&entries_counter,data+sizeof(char),sizeof(int));
	key1 = malloc(size1);
	key2 = malloc(size1);
	
	data = data + (sizeof(char)+2*sizeof(int));
	data = data + (break_point*(size1+size2));
	memcpy(key1,data,size1);
	data1 = data - (size1+size2);
	memcpy(key2,data1,size1);
	while(key_compare(key1,key2,OpenFile[indexDesc].attributes.attrType1)==0 && (break_point < entries_counter))	//Check to the right 
	{
		break_point = break_point + 1;
		data = data + (size1+size2);
		memcpy(key2,data,size1);
	}
	memcpy(key2,data1,size1);
	while((break_point == entries_counter) && (counter>0) && (key_compare(key1,key2,OpenFile[indexDesc].attributes.attrType1)==0))//Check to the left
	{
		counter = counter - 1;
		data1 = data1 - (size1+size2);
		memcpy(key2,data1,size1);
	}
	free(key1);
	free(key2);
	if (break_point == entries_counter) return counter;     //Block is full of the same value and 0 is returned
	else return break_point;							  //New break point is returned 
}

//This function finds the appropriate block that this specific key should be inserted.
int Search_Block(void* key,stack_node** stack,int indexDesc)
{
	int keys_num=0,temp_ptr,flag=0,original_keys_num;
	char* data;
	char descriptory = 'i';
	void* temp_key;
	int size1 = OpenFile[indexDesc].attributes.attrLength1;
	int fd = OpenFile[indexDesc].filedesc;
	int root = OpenFile[indexDesc].root;
	int index_block_num = root;
	BF_Block* block;

	temp_key = malloc(size1);
	BF_Block_Init(&block);
	CALL_BF(BF_GetBlock(fd,root,block));
	data = BF_Block_GetData(block); 

	while(descriptory == 'i') 														//While we are at an index block
	{
		push(stack,index_block_num);
		memcpy(&original_keys_num,data+sizeof(char),sizeof(int));
		keys_num = original_keys_num;
		data = data + (sizeof(char)+2*sizeof(int));

		while(keys_num)														   //Check its keys one by one
		{
			memcpy(temp_key,data,size1);
			if(key_compare(temp_key,key,OpenFile[indexDesc].attributes.attrType1) > 0)   //If we found a key that is bigger the block we need is at its left pointer
			{
				
				memcpy(&temp_ptr,data-sizeof(int),sizeof(int));				   //Get the left pointer of the key
				CALL_BF(BF_UnpinBlock(block));
				
				if (temp_ptr == 0)											   //If there is no leaf block here error
				{
					BF_Block_Destroy(&block);
					return -1;													
				}
				CALL_BF(BF_GetBlock(fd,temp_ptr,block));				//Get the new block
				index_block_num = temp_ptr;
				data = BF_Block_GetData(block);
				memcpy(&descriptory,data,sizeof(char));							//Get its block descriptory

				break;															//Since we finished with this block, stop this loop

			}
			data = data + size1;
			keys_num = keys_num -1;
			if (keys_num==0)													//If we reached the end of the index block
			{
				memcpy(&temp_ptr,data,sizeof(int));

				CALL_BF(BF_UnpinBlock(block));										
						
				CALL_BF(BF_GetBlock(fd,temp_ptr,block));				//Get the new block
				index_block_num = temp_ptr;
				data = BF_Block_GetData(block);
				memcpy(&descriptory,data,sizeof(char));							//Get its block descriptory
				
				break;
			}
			data = data + sizeof(int);
		}
	}
	CALL_BF(BF_UnpinBlock(block));
	BF_Block_Destroy(&block);
	free(temp_key);
	return temp_ptr;
}

//Returns the first data block of an AM file.
int first_data_block(int indexDesc)
{
    int first_child;
	char descriptory = 'i';
    char* data;
    BF_Block *block;
    
	if(OpenFile[indexDesc].root==0)
    	return 1;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,OpenFile[indexDesc].root,block));
    data = BF_Block_GetData(block);
    CALL_BF(BF_UnpinBlock(block));                              
    
	//While we are in index blocks.
	while(descriptory=='i')
    {
        data = data + (sizeof(char)+sizeof(int));
        memcpy(&first_child,data,sizeof(int));
        CALL_BF(BF_GetBlock(OpenFile[indexDesc].filedesc,first_child,block));
        data = BF_Block_GetData(block);
        memcpy(&descriptory,data,sizeof(char));
        CALL_BF(BF_UnpinBlock(block));
    }
    BF_Block_Destroy(&block);
    return first_child;
}

//Compares a key and value depending to operator.
int OS_Compare(void *key,void *value,int op,char type)
{
    int result;
    result = key_compare(key,value,type);
    switch(op)
    {
		case 1:
        	if(result == 0) return 1;
        	else return 0;
		case 2:
			if(result != 0) return 1;
        	else return 0;
		case 3:
			if(result == -1) return 1;
			else return 0;
		case 4:
			if(result == 1) return 1;
			else return 0;
		case 5:
			if(result == -1 || result == 0) return 1;
			else return 0;  
		case 6:
			if(result == 1 || result == 0) return 1;
			else return 0;
	}
}


/////////////////////////////////////////////////////
///////////PRINTING...JUST FOR DEBUGGING////////////
///////////////////////////////////////////////////
void print_index_block(char* data,int indexDesc)
{
	int recs_num;
	memcpy(&recs_num,data+sizeof(char),sizeof(int));
	printf("##########################################################\npairs %d \n",*(int*)(data+sizeof(char)));
	printf("pointer %d ",*(int*)(data+sizeof(char)+sizeof(int)));
	data = data + (sizeof(char)+2*sizeof(int));
	while(recs_num)
	{
		if(OpenFile[indexDesc].attributes.attrType1=='i')
			printf("key %d  pointer %d\n",*(int*)data,*(int*)(data+sizeof(int)));
		else if(OpenFile[indexDesc].attributes.attrType1=='c'){printf("key %s pointer %d\n",data,*(int*)(data+OpenFile[indexDesc].attributes.attrLength1));}
		else printf("key %.2f pointer %d\n",*(float*)data,*(int*)(data+OpenFile[indexDesc].attributes.attrLength1));
		data = data + (OpenFile[indexDesc].attributes.attrLength1+sizeof(int));
		recs_num = recs_num - 1;
	}
}

void print_data_block(char* data,int indexDesc)
{
	int recs_num;
	memcpy(&recs_num,data+sizeof(char),sizeof(int));
		printf("TYPE : %d\n",*(char*)data);

	data = data + sizeof(char)+2*sizeof(int);

	while(recs_num)
	{
		if(OpenFile[indexDesc].attributes.attrType1=='i')printf("key %d ",*(int*)data);
		else if(OpenFile[indexDesc].attributes.attrType1=='c')printf("key %s ",data);
		else printf("key %.2f ",*(float*)data);
		
		
		if(OpenFile[indexDesc].attributes.attrType2=='i') printf("value %d\n",*(int*)(data+OpenFile[indexDesc].attributes.attrLength1));
		else if(OpenFile[indexDesc].attributes.attrType2=='c')printf("value %s\n",(data+OpenFile[indexDesc].attributes.attrLength1));
		else printf("value %.2f\n",*(float*)(data+OpenFile[indexDesc].attributes.attrLength1));
		data= data+OpenFile[indexDesc].attributes.attrLength1+OpenFile[indexDesc].attributes.attrLength2;
		recs_num= recs_num -1;
	}
}
