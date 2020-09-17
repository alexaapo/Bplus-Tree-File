#include "bf.h"

extern int AM_errno;

#define AME_OK 0
#define AME_ERROR -1
#define AME_BF_ERROR -2
#define AME_FILE_NOT_FOUND -3
#define AME_FILE_ALREADY_EXISTS -4
#define AME_TYPE_LENGTH_NOT_MATCH -5
#define AME_DESTROY_WITH_OPEN_FILES -6
#define AME_DESTROY_WITH_OPEN_SCAN_FILES -7
#define AME_WRONG_FILE_TYPE -8
#define AME_OPEN_SCAN_FOR_THIS_FILE -9
#define AME_EOF -10
#define AME_NO_SPACE_IN_FILES_ARRAY -11
#define AME_SCAN_NOT_FOUND -12
#define AME_BLOCK_NOT_FOUND -13
#define AME_NO_SCAN_SPACE -14
#define AME_SCAN_VALUE_NOT_EXISTS -15
#define UNCOMPATIBLE_TYPE_OF_FILE -16



#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4 
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6


#define MAX_OPEN_FILES 20
#define MAX_SCAN_FILES 20

typedef struct Attributes
{
  char attrType1;
  int attrLength1;
  char attrType2;
  int attrLength2;
} Attributes;

//Global Array OpenFile is made up from these structs.
typedef struct Fileindex {
  int filedesc;
  int root;
  int openscans;
  int max_entries;
  int max_pairs;
	short int flag;
	char filename[20];
  Attributes attributes;
} Fileindex;

//Global Array OpenScan is made up from these structs.
typedef struct Scanindex {
  int indexdesc;
  short int flag;
  void* value;
  int op;
  int start_block;
	int record; //counter 
} Scanindex;

//Global Arrays 
Fileindex OpenFile[MAX_OPEN_FILES];
Scanindex OpenScan[MAX_SCAN_FILES];

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void *AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);

void AM_Close();

////////////////////////////////////////////
///////FUNCTIONS JUST FOR DEBUGGING////////
//////////////////////////////////////////
int tsoytsos(int ,int);
int indextsoutsos(int );
