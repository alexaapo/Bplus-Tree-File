#include "bf.h"

typedef struct Stack_node {
    int data;
    struct Stack_node* next;
} stack_node;

int push(stack_node** ,int);
int pop(stack_node**);
void delete_stack(stack_node*);

int Entries_in_dataBlock(int, int);
int Pairs_in_indexBlock(int);
int Create_Block(BF_Block **, char, int);
int Pos_Of_Entry_In_Block(char* , void* , int);
int key_compare(void* ,void* , char);
void Sort_Entries_In_Block(char* , void* , void* , int);
int Break_Block(char* ,void* ,void* , void** ,int);
int Same_Keys(char* ,int , int);
int Search_Block(void* ,stack_node**,int);
int first_data_block(int);
int OS_Compare(void *,void *,int ,char);

void print_index_block(char* ,int );
void print_data_block(char* ,int );