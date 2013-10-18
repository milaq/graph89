/*
 *   Graph89 - Emulator for Android
 *	 Copyright (C) 2012-2013  Dritan Hashorva
 *
 *   WabbitEmu
 *   Copyright (C)  http://wabbit.codeplex.com/
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#define BOOL unsigned char
#define BYTE unsigned char
#define DWORD unsigned short
#define u_char unsigned char
#define u_int unsigned int
#define TCHAR char
#define _tcsicmp strcasecmp
#define _tcscpy_s strcpy
#define _tcsrchr strrchr
#define _strnicmp strncasecmp
#define ZeroMemory(dest, size) memset(dest, 0, size)
#define _T(z) z
#define tmpread( fp ) \
	tmp = fgetc(fp); \
	if (tmp == EOF) { \
		fclose(fp); \
		FreeTiFile(tifile); \
		return NULL; \
	}
#define ARRAYSIZE(z) (sizeof(z)/sizeof((z)[0]))


// Destination flags
typedef enum SEND_FLAG {
	SEND_CUR,					/* sends based on current flag settings */
	SEND_RAM,					/* sends to RAM, regardless of flag settings */
	SEND_ARC					/* sends to archive, regardless of flag settings */
} SEND_FLAG;


typedef struct {
	char tag[4];
	int pnt;
	int size;
	unsigned char *data;
} CHUNK_t;


typedef struct {
	int version_major;
	int version_minor;
	int version_build;
	int model;
	int chunk_count;
	char author[32];
	char comment[64];
	CHUNK_t* chunks[512];
} SAVESTATE_t;


typedef struct INTELHEX {
	int DataSize;
	int Address;
	int Type;
	unsigned char Data[256];
	int CheckSum;
} INTELHEX_t;

#pragma pack(1)
typedef struct TIFLASH {
	unsigned char sig[8];
	unsigned char rev[2];
	unsigned char flag;
	unsigned char object;
	unsigned char date[4];
	unsigned char namelength;
	unsigned char name[8];
	unsigned char filler[23];
	unsigned char device;
	unsigned char type;
	unsigned char filler2[24];
	unsigned int hexsize;
//	int rpage[256];
	int pagesize[256];
	unsigned char *data[256];
//	unsigned short chksum;

	unsigned int pages;		//total number of pages.

} TIFLASH_t;

typedef struct ROM {
	int size;
	char version[32];
	unsigned char *data;
} ROM_t;

typedef struct TIBACKUP {
	unsigned short headersize;// size of the header up to name, sometimes ignored
	unsigned short length1;			// data size
	unsigned char vartype;			// what type of varible
	unsigned short length2;			// data size
	unsigned short length3;			// data size
	unsigned short address;			// duplicate of data size

	unsigned short length1a;		// Repeats of the data length.
	unsigned char *data1;			// pointer to data

	unsigned short length2a;		// data size
	unsigned char *data2;			// pointer to data

	unsigned short length3a;		// data size
	unsigned char *data3;			// pointer to data

} TIBACKUP_t;

typedef struct TIVAR {
	unsigned short headersize;// size of the header up to name, sometimes ignored
	unsigned short length;			// data size
	unsigned char vartype;			// what type of variable
	unsigned char name_length;		// 85/86 only name length is variable
	unsigned char name[8];			// null padded name
	unsigned char version;			// 0 83+only
	unsigned char flag;				// bit 7 is if flash 83+only
	unsigned short length2;			// duplicate of data size
	unsigned char *data;			// pointer to data
} TIVAR_t;

typedef struct TIFILE {
	unsigned char sig[8];
	unsigned char subsig[3];
	unsigned char comment[42];
	unsigned char length;
	TIVAR_t *var;
	TIVAR_t *vars[256];
	unsigned char chksum;
	int model;
	int type;
	ROM_t *rom;
	TIFLASH_t *flash;
	SAVESTATE_t *save;
	TIBACKUP_t *backup;
} TIFILE_t;

#pragma pack()

#define TI_FLASH_HEADER_SIZE 8+2+1+1+4+1+8+23+1+1+24+4
#define TI_FILE_HEADER_SIZE 8+3+42/*+2*/
#define TI_VAR_HEADER_SIZE 2+2+1+8

#define ROM_TYPE	1		//Rom
#define FLASH_TYPE	2		//Flash application or OS
#define VAR_TYPE	3		//most varibles can be supported under an umbrella type
#define SAV_TYPE	4		//Wabbit specific saves.
#define BACKUP_TYPE	5		//Wabbit specific saves.
#define LABEL_TYPE	6		//Lab file
#define BREAKPOINT_TYPE 7	//breakpoint file
#define GROUP_TYPE 	8		//groups are stored weirdly so they get a weird type
#define ZIP_TYPE 	9		//zip/tig file
#define FLASH_TYPE_OS 0x23
#define FLASH_TYPE_APP 0x24

#define FLASH_HEADER	"**TIFL**"

#define TI_81		0
#define TI_82		1
#define TI_83		2
#define TI_85		3
#define TI_86		4
#define TI_73		5
#define TI_83P		6
#define TI_83PSE	7
#define TI_84P		8
#define TI_84PSE	9



/*defines the start of the app page*/
/*this page starts in HIGH mem and grows to LOW */
#define TI_73_APPPAGE		0x15	/*Ummm...*/
#define TI_83P_APPPAGE		0x15
#define TI_83PSE_APPPAGE	0x69
#define TI_84P_APPPAGE		0x29	/*Ummm...*/
#define TI_84PSE_APPPAGE	0x69


/*defines the Number of user archive pages*/
#define TI_73_USERPAGES		0x0A	/*Ummm...*/
#define TI_83P_USERPAGES	0x0A
#define TI_83PSE_USERPAGES	0x60
#define TI_84P_USERPAGES	0x1E	/*Ummm...*/
#define TI_84PSE_USERPAGES	0x60




#define PAGE_SIZE_W 16384

// Link errors
typedef enum {
	LERR_SUCCESS = 0,			/* No error */
	LERR_LINK = 1,				/* General link error */
	LERR_TIMEOUT,				/* Time out error */
	LERR_FORCELOAD,				/* Error force loading an application (TI-83+) */
	LERR_CHKSUM,				/* Packet with invalid checksum was received */
	LERR_NOTINIT,				/* Link was not initialized */
	LERR_MEM,					/* Not enough memory on calc */
	LERR_MODEL,					/* Not the correct model for file */
	LERR_FILE,					/* Invalid TIFILE in argument */
	LERR_SYSTEM,				/* Something wrong in wabbitemu */
	LERR_TURN_ON,				/* We need to turn on because a ROM image was sent */
} LINK_ERR;

typedef struct upages {
	u_int start, end;
} upages_t;
typedef struct apphdr {
	TCHAR name[12];
	u_int page, page_count;
} apphdr_t;
typedef struct applist {
	u_int count;
	apphdr_t apps[96];
} applist_t;


TIFILE_t* importvar(const char * filePath, BOOL only_check_header);
void calc_erase_certificate(unsigned char *mem, int size);
