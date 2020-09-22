# B+-Tree-File
For this project, I implemented an access method based on the B +trees. I organized a two-field file in B+
trees, using the first field of the file as the key-field. Each file that corresponds
in a B + tree will be made of blocks, which I organize properly to have
the structure required to function as it should be. Each block of the file has a size
BLOCKSIZE = 512 bytes.

Futhermore, the difficult part in this project was the implementation of insertion of a node in a B+ tree, cause we have to search or sort entries in blocks or maybe break blocks. Important info is that we can differ index blocks from leaf nodes (data blocks) from their first byte which is 'i' or 'l' respectively. Finally our B+ Tree can deal with we have a lot of Entries with same keys, with a function Same_Keys() which calculates in which point will break old block into two new.

Some of the fuctions that I implement:

- AM_Init()

- int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2)

- int AM_DestroyIndex(char *fileName)

- int AM_OpenIndex(char *fileName)

- int AM_CloseIndex (int fileDesc)

- int AM_InsertEntry(int fileDesc, void *value1, void *value2)

- int AM_OpenIndexScan(int fileDesc, int op, void *value)

- void *AM_FindNextEntry(int scanDesc)

- int AM_CloseIndexScan(int scanDesc)

- void AM_PrintError(char *errString)

- void AM_Close()

## Execution:
1) **bash script1.sh** (Check results of first queries)

2) **bash script2.sh** (Check results of next queries)

3) **bash script3.sh** (Clean what we have just created)

