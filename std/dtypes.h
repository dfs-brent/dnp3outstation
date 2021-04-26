#ifndef DFS_DTYPES_H
#define DFS_DTYPES_H

#include "constants.h"

/* Data types */

/* single byte */
typedef unsigned char BYTE;
typedef unsigned char FLAG;
typedef unsigned short int WORD;

/* positive value whole numbers */
typedef unsigned int  UINT;
typedef unsigned char TINYINT;
typedef unsigned short int  SMALLINT;
typedef unsigned int  MEDIUMINT;
typedef unsigned long int   BIGINT;

/* signed numbers */
typedef signed char TINYNUM;
typedef signed short int  SMALLNUM;
typedef signed int  MEDIUMNUM;
typedef signed long int   BIGNUM;


/* Buffers */

/* max size 0-SMALLINT */
typedef struct {
	SMALLINT size;
	BYTE *buf;
}BUFFER;

/* max size 0-SMALLINT */
typedef struct {
	SMALLINT size;
	char *buf;
}CHAR_BUFFER;

#endif
