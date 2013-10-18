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


#include <stdio.h>
#include <stdlib.h>
#include <wabbit.h>
#include <tilem.h>

const char self_test[] = "Self Test?";
const char catalog[] = "CATALOG";
const char txt73[] = "GRAPH  EXPLORER  SOFTWARE";
const char txt86[] = "Already Installed";

static TIFILE_t* ImportVarData(FILE *infile, TIFILE_t *tifile, int varNumber);
static TIFILE_t* ImportROMFile(FILE *infile, TIFILE_t *tifile);
static TIFILE_t* ImportFlashFile(FILE *infile, TIFILE_t *tifile);
static TIFILE_t* ImportBackup(FILE *infile, TIFILE_t *tifile);
static int FindRomVersion(int calc, char *string, unsigned char *rom,
		u_int size);
static int ReadIntelHex(FILE *ifile, INTELHEX_t *ihex);

static int CmpStringCase(const char *str1, unsigned char *str2) {
	return _strnicmp(str1, (char *) str2, strlen(str1));
}

static void NullTiFile(TIFILE_t* tifile) {
	tifile->var = NULL;			//make sure its null. mostly for freeing later
	memset(tifile->vars, 0, sizeof(tifile->vars));
	tifile->flash = NULL;
	tifile->rom = NULL;
	tifile->save = NULL;
	tifile->backup = NULL;
	tifile->type = VAR_TYPE;
}

static TIFILE_t* InitTiFile() {
	TIFILE_t *tifile = (TIFILE_t*) malloc(sizeof(TIFILE_t));
	if (tifile == NULL)
		return NULL;
	NullTiFile(tifile);
	return tifile;
}

TIFILE_t* FreeTiFile(TIFILE_t * tifile) {
	if (!tifile)
		return NULL;

	int i = 0;
	while (tifile->vars[i] != NULL) {
		if (tifile->vars[i]->data)
			free(tifile->vars[i]->data);
		free(tifile->vars[i]);
		tifile->vars[i] = NULL;
		i++;
	}
	if (tifile->flash) {
		int i;
		for (i = 0; i < 256; i++) {
			if (tifile->flash->data[i])
				free(tifile->flash->data[i]);
		}
		free(tifile->flash);
	}
	if (tifile->rom) {
		if (tifile->rom->data)
			free(tifile->rom->data);
		free(tifile->rom);
	}
	free(tifile);
	return NULL;
}

static void ReadTiFileHeader(FILE *infile, TIFILE_t *tifile) {
	char headerString[8];
	int i, tmp;

	fread(headerString, 1, 8, infile);
	rewind(infile);

	if (!_strnicmp(headerString, FLASH_HEADER, 8)) {
		tifile->type = FLASH_TYPE;
		tifile->flash = (TIFLASH_t*) malloc(sizeof(TIFLASH_t));
		ZeroMemory(tifile->flash, sizeof(TIFLASH_t));
		if (tifile->flash == NULL) {
			FreeTiFile(tifile);
			return;
		}

		unsigned char *ptr = (unsigned char *) tifile->flash;
		for (i = 0; i < TI_FLASH_HEADER_SIZE && !feof(infile); i++) {
			tmp = fgetc(infile);
			if (tmp == EOF) {
				fclose(infile);
				FreeTiFile(tifile);
				return;
			}
			ptr[i] = tmp;
		}
		return;
	}

	/* It maybe a rom if it doesn't have the Standard header */
	if (_strnicmp(headerString, "**TI73**", 8)
			&& _strnicmp(headerString, "**TI82**", 8)
			&& _strnicmp(headerString, "**TI83**", 8)
			&& _strnicmp(headerString, "**TI83F*", 8)
			&& _strnicmp(headerString, "**TI85**", 8)
			&& _strnicmp(headerString, "**TI86**", 8)) {
		tifile->type = ROM_TYPE;
		return;
	}

	/* Import file Header */
	unsigned char *ptr = (unsigned char *) tifile;
	for (i = 0; i < TI_FILE_HEADER_SIZE && !feof(infile); i++) {
		tmp = fgetc(infile);
		if (tmp == EOF) {
			FreeTiFile(tifile);
			return;
		}
		ptr[i] = tmp;
	}

	if (!_strnicmp((char *) tifile->sig, "**TI73**", 8))
		tifile->model = TI_73;
	else if (!_strnicmp((char *) tifile->sig, "**TI82**", 8))
		tifile->model = TI_82;
	else if (!_strnicmp((char *) tifile->sig, "**TI83**", 8))
		tifile->model = TI_83;
	else if (!_strnicmp((char *) tifile->sig, "**TI83F*", 8))
		tifile->model = TI_83P;
	else if (!_strnicmp((char *) tifile->sig, "**TI85**", 8))
		tifile->model = TI_85;
	else if (!_strnicmp((char *) tifile->sig, "**TI86**", 8))
		tifile->model = TI_86;
	else {
		FreeTiFile(tifile);
		return;
	}
	return;
}

TIFILE_t* importvar(const char * filePath, BOOL only_check_header) {
	FILE *infile = NULL;
	TIFILE_t *tifile;

	char extension[5] = _T("");
	const char *pext = _tcsrchr(filePath, _T('.'));
	if (pext != NULL) {
		_tcscpy_s(extension, pext);
	}

	tifile = InitTiFile();
	if (tifile == NULL) {
		return NULL;
	}

	if (!_tcsicmp(extension, _T(".lab"))) {
		tifile->type = LABEL_TYPE;
		return tifile;
	}

	if (!_tcsicmp(extension, _T(".brk"))) {
		tifile->type = BREAKPOINT_TYPE;
		return tifile;
	}

#ifdef _WINDOWS
	if (!_tcsicmp(extension, _T(".tig")) || !_tcsicmp(extension, _T(".zip")) ) {
		tifile->type = ZIP_TYPE;
		if (!only_check_header) {
			ImportZipFile(filePath, tifile);
		}
		return tifile;
	}
#endif

	infile = fopen(filePath, "rb");
	if (infile == NULL) {
		return FreeTiFile(tifile);
	}

	ReadTiFileHeader(infile, tifile);

	if (only_check_header && tifile->type != ROM_TYPE) {
		fclose(infile);
		return tifile;
	}

	tifile = ImportVarData(infile, tifile, 0);
	fclose(infile);
	return tifile;
}

static short length2 = 0;
static TIFILE_t* ImportVarData(FILE *infile, TIFILE_t *tifile, int varNumber) {
	switch (tifile->type) {
	case ROM_TYPE:
		return ImportROMFile(infile, tifile);
	case FLASH_TYPE:
		return ImportFlashFile(infile, tifile);
	}

	int i, tmp;
	unsigned short headersize;
	unsigned short length;
	unsigned char vartype, *ptr;

	if (varNumber == 0) {
		tmpread(infile);
		length2 = tmp;
		tmpread(infile);
		length2 += tmp << 8;
	}

	tmpread(infile);
	headersize = tmp;
	tmpread(infile);
	headersize += tmp << 8;

	tmpread(infile);
	length = tmp;
	tmpread(infile);
	length += tmp << 8;

	tmpread(infile);
	vartype = tmp;

	if ((tifile->model == TI_73 && vartype == 0x13)
			|| (tifile->model == TI_82 && vartype == 0x0F)
			|| (tifile->model == TI_85 && vartype == 0x1D)) {
		tifile->backup = (TIBACKUP_t *) malloc(sizeof(TIBACKUP_t));
		if (tifile->backup == NULL)
			return FreeTiFile(tifile);
		tifile->backup->headersize = headersize;
		tifile->backup->length1 = length;
		tifile->backup->vartype = vartype;
		return ImportBackup(infile, tifile);
	}

	if (length2 > length + 17 || tifile->type == GROUP_TYPE) {
		tifile->type = GROUP_TYPE;
	} else {
		tifile->type = VAR_TYPE;
	}

	tifile->var = (TIVAR_t *) malloc(sizeof(TIVAR_t));
	tifile->vars[varNumber] = tifile->var;
	if (tifile->var == NULL)
		return FreeTiFile(tifile);

	char name_length = 8;
	if (tifile->model == TI_86 || tifile->model == TI_85) {
		//skip name length
		name_length = tmpread(infile)
		;
		if (tifile->model == TI_86) {
			name_length = 8;
		}
	}

	tifile->var->name_length = name_length;
	tifile->var->headersize = headersize;
	tifile->var->length = length;
	tifile->var->vartype = vartype;
	ptr = tifile->var->name;
	for (i = 0; i < name_length && !feof(infile); i++) {
		tmpread(infile);
		ptr[i] = tmp;
	}

	if (tifile->model == TI_83P) {
		tmp = fgetc(infile);
		if (tmp == EOF) {
			fclose(infile);
			FreeTiFile(tifile);
			return NULL;
		}
		if (tmp > 5) {
			//_putts(_T("Warning version is greater than 5, setting to 0"));
			tmp = 0;
		}
		ptr[i++] = tmp;
		tmp = fgetc(infile);
		if (tmp == EOF)
			return FreeTiFile(tifile);
		ptr[i++] = tmp;
	} else {
		ptr[i++] = 0;
		ptr[i++] = 0;
	}
	tmp = fgetc(infile);
	if (tmp == EOF)
		return FreeTiFile(tifile);
	ptr[i++] = tmp;
	tmp = fgetc(infile);
	if (tmp == EOF)
		return FreeTiFile(tifile);
	ptr[i++] = tmp;

	tifile->var->data = (unsigned char *) malloc(tifile->var->length);
	if (tifile->var->data == NULL) {
		return FreeTiFile(tifile);
	}

	for (i = 0; i < tifile->var->length && !feof(infile); i++) {
		tmp = fgetc(infile);
		if (tmp == EOF) {
			return FreeTiFile(tifile);
		}
		tifile->var->data[i] = tmp;
	}

	if (tifile->type == GROUP_TYPE) {
		if (varNumber != 0) {
			return tifile;
		}
		while (tifile != NULL) {
			length2 -= tifile->var->length + 17;
			if (length2 <= 0) {
				break;
			}
			tifile = ImportVarData(infile, tifile, ++varNumber);
		}
	}

	tifile->chksum = (fgetc(infile) & 0xFF) + ((fgetc(infile) & 0xFF) << 8);
	return tifile;
}

static TIFILE_t* ImportFlashFile(FILE *infile, TIFILE_t *tifile) {
	int i;
	for (i = 0; i < 256; i++) {
		tifile->flash->pagesize[i] = 0;
		tifile->flash->data[i] = NULL;
	}

	INTELHEX_t record;
	int CurrentPage = -1;
	int HighestAddress = 0;
	int TotalSize = 0;
	int TotalPages = 0;
	int done = 0;

	if (tifile->flash->type == FLASH_TYPE_OS) {
		// Find the first page, usually after the first line
		do {
			if (!ReadIntelHex(infile, &record)) {
				FreeTiFile(tifile);
				return NULL;
			}
		} while (record.Type != 0x02 || record.DataSize != 2);
		CurrentPage = ((record.Data[0] << 8) | record.Data[1]) & 0x7F;
		if (tifile->flash->data[CurrentPage] == 0) {
			tifile->flash->data[CurrentPage] = (unsigned char *) malloc(
			PAGE_SIZE_W);
			if (tifile->flash->data[CurrentPage] == NULL) {
				FreeTiFile(tifile);
				return NULL;
			}
			memset(tifile->flash->data[CurrentPage], 0, PAGE_SIZE_W);//THIS IS IMPORTANT FOR LINKING, APPS FOR NOW
		}
		HighestAddress = 0;
	}

	while (!feof(infile) && !done) {
		ReadIntelHex(infile, &record);

		switch (record.Type) {
		case 00:
			if (CurrentPage > -1) {
				for (i = 0;
						(i < record.DataSize)
								&& (((i + record.Address) & 0x3FFF)
										< PAGE_SIZE_W); i++) {
					tifile->flash->data[CurrentPage][(i + record.Address)
							& 0x3FFF] = record.Data[i];
				}
				if (HighestAddress < i + record.Address)
					HighestAddress = (int) (i + record.Address);
			}
			break;
		case 01:
			done = 1;
			if (CurrentPage == -1) {
				printf("invalid current page\n");
				FreeTiFile(tifile);
				return NULL;
			}
			TotalSize += (HighestAddress - PAGE_SIZE_W);
			tifile->flash->pagesize[CurrentPage] =
					(HighestAddress - PAGE_SIZE_W);
			tifile->flash->pages = TotalPages;
			break;
		case 02:
			if (CurrentPage > -1) {
				TotalSize += PAGE_SIZE_W;
				tifile->flash->pagesize[CurrentPage] = (HighestAddress
						- PAGE_SIZE_W);
			}
			TotalPages++;
			CurrentPage = ((record.Data[0] << 8) | record.Data[1]) & 0x7F;
			if (tifile->flash->data[CurrentPage] == 0) {
				tifile->flash->data[CurrentPage] = (unsigned char *) malloc(
				PAGE_SIZE_W);
				if (tifile->flash->data[CurrentPage] == NULL) {
					FreeTiFile(tifile);
					return NULL;
				}
				memset(tifile->flash->data[CurrentPage], 0, PAGE_SIZE_W);//THIS IS IMPORTANT FOR LINKING, APPS FOR NOW
			}
			HighestAddress = 0;
			break;
		default:
			printf("unknown record\n");
			FreeTiFile(tifile);
			return NULL;
		}
	}

	if (tifile->flash->device == 0x74) {
		tifile->model = TI_73;
	} else if (tifile->flash->device == 0x73) {
		tifile->model = TI_83P;
	} else {
		FreeTiFile(tifile);
		return NULL;
	}
	return tifile;
}

static TIFILE_t* ImportROMFile(FILE *infile, TIFILE_t *tifile) {
	size_t size;
	int calc, tmp;
	u_int i;

	fseek(infile, 0, SEEK_END);
	size = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	if (size == 32 * 1024)
		calc = TI_81;
	else if (size == 128 * 1024)
		calc = TI_82;
	else if (size == 256 * 1024)
		calc = TI_83;
	else if ((size >= 510 * 1024) && (size <= (590 * 1024)))
		calc = TI_83P;
	else if ((size >= 1016 * 1024) && (size <= (1030 * 1024)))
		calc = TI_84P;
	else if ((size >= 2044 * 1024) && (size <= (2260 * 1024)))
		calc = TI_83PSE;
	else {
		puts("not a known rom");
		return FreeTiFile(tifile);
	}

	tifile->rom = (ROM_t *) malloc(sizeof(ROM_t));
	if (tifile->rom == NULL) {
		return FreeTiFile(tifile);
	}

	tifile->rom->data = (unsigned char *) malloc(size);
	if (tifile->rom->data == NULL)
		return FreeTiFile(tifile);

	for (i = 0; i < size && !feof(infile); i++) {
		tmp = fgetc(infile);
		if (tmp == EOF)
			return FreeTiFile(tifile);
		tifile->rom->data[i] = tmp;
	}
	tifile->rom->size = (int) size;
	calc = FindRomVersion(calc, tifile->rom->version, tifile->rom->data,
			(int) size);
	tifile->model = calc;

	return tifile;
}

static int FindRomVersion(int calc, char *string, unsigned char *rom,
		u_int size) {
	u_int i;
	int b;
	if (calc == -1) {
		if (size == (128 * 1024))
			calc = TI_82;
		else if (size == (256 * 1024))
			calc = TI_83;
		else if ((size >= (510 * 1024)) && (size <= (590 * 1024)))
			calc = TI_83P;
		else if ((size >= (1016 * 1024)) && (size <= (1030 * 1024)))
			calc = TI_84P;
		else if ((size >= (2044 * 1024)) && (size <= (2260 * 1024)))
			calc = TI_83PSE;
		else {
//			_putts(_T("not a known rom"));
			return -1;
		}
	}
	switch (calc) {
	case TI_81:
		//1.1k doesnt ld a,* first
		if (rom[0] == 0xC3) {
			string[0] = '1';
			string[1] = '.';
			string[2] = '1';
			string[3] = 'K';
			string[4] = '\0';
		} else if (rom[1] == 0x17) {
			//2.0V has different val to load
			string[0] = '2';
			string[1] = '.';
			string[2] = '0';
			string[3] = 'V';
			string[4] = '\0';
		} else if (rom[5] == 0x09) {
			//1.6K outs a 0x09
			string[0] = '1';
			string[1] = '.';
			string[2] = '6';
			string[3] = 'K';
			string[4] = '\0';
		} else {
			//assume its a 1.8K for now
			string[0] = '1';
			string[1] = '.';
			string[2] = '8';
			string[3] = 'K';
			string[4] = '\0';
		}
		break;
	case TI_82:
		for (i = 0; i < (size - strlen(catalog) - 10); i++) {
			if (!CmpStringCase(catalog, rom + i)) {
				calc = TI_85;
				for (i = 0; i < (size - strlen(self_test) - 10); i++) {
					if (CmpStringCase(self_test, rom + i) == 0)
						break;
				}
				for (; i < (size - 40); i++) {
					if (isdigit(rom[i]))
						break;
				}
				if (i < (size - 40)) {
					for (b = 0; (b + i) < (size - 4) && b < 32; b++) {
						if (rom[b + i] != ' ')
							string[b] = rom[b + i];
						else
							string[b] = 0;
					}
					string[31] = 0;
				} else {
					string[0] = '?';
					string[1] = '?';
					string[2] = '?';
					string[3] = 0;
				}
				break;
			}
		}
		if (calc != TI_82)
			break;

	case TI_83:
	case TI_86:
		for (i = 0; i < (size - strlen(txt86) - 10); i++) {
			if (!CmpStringCase(txt86, rom + i)) {
				calc = TI_86;
				for (i = 0; i < size - strlen(self_test) - 10; i++) {
					if (CmpStringCase(self_test, rom + i) == 0)
						break;
				}
				for (; i < size - 40; i++) {
					if (isdigit(rom[i]))
						break;
				}
				if (i < size - 40) {
					for (b = 0; (b + i) < (size - 4) && b < 32; b++) {
						if (rom[b + i] != ' ')
							string[b] = rom[b + i];
						else
							string[b] = 0;
					}
					string[31] = 0;
				} else {
					string[0] = '?';
					string[1] = '?';
					string[2] = '?';
					string[3] = 0;
				}
				break;
			}
		}

		if (calc == TI_86) {
			break;
		}

		for (i = 0; i < (size - strlen(self_test) - 10); i++) {
			if (CmpStringCase(self_test, rom + i) == 0)
				break;
		}
		if ((i + 64) < size) {
			i += 10;
			for (b = 0; b < 32; b++) {
				string[b] = rom[i++];
			}
			string[31] = 0;
		} else {
			string[0] = '?';
			string[1] = '?';
			string[2] = '?';
			string[3] = 0;
		}
		break;
	case TI_83P:
		for (i = 0; i < (size - strlen(txt73) - 10); i++) {
			if (CmpStringCase(txt73, rom + i) == 0) {
				calc = TI_73;
				break;
			}
		}
	case TI_84P:
	case TI_83PSE:
	case TI_84PSE:
		i = 0x0064;
		for (b = 0; b < 32; b++) {
			string[b] = rom[i++];
		}
		string[31] = 0;
		if (calc == TI_83PSE) {
			if (string[0] > '1') {
				calc = TI_84PSE;
			}
		}
		break;
	}
	return calc;
}

static TIFILE_t* ImportBackup(FILE *infile, TIFILE_t *tifile) {
	int i, tmp;
	tifile->backup->data1 = NULL;
	tifile->backup->data2 = NULL;
	tifile->backup->data3 = NULL;

	tmpread(infile);
	tifile->backup->length2 = tmp;
	tmpread(infile);
	tifile->backup->length2 += tmp << 8;

	tmpread(infile);
	tifile->backup->length3 = tmp;
	tmpread(infile);
	tifile->backup->length3 += tmp << 8;

	tmpread(infile);
	tifile->backup->address = tmp;
	tmpread(infile);
	tifile->backup->address += tmp << 8;

	tmpread(infile);
	tifile->backup->length1a = tmp;
	tmpread(infile);
	tifile->backup->length1a += tmp << 8;

	tifile->backup->data1 = (unsigned char *) malloc(tifile->backup->length1);
	if (tifile->backup->data1 == NULL)
		return FreeTiFile(tifile);
	for (i = 0; i < tifile->backup->length1 && !feof(infile); i++) {
		tmpread(infile);
		tifile->backup->data1[i] = tmp;
	}

	tmpread(infile);
	tifile->backup->length2a = tmp;
	tmpread(infile);
	tifile->backup->length2a += tmp << 8;

	tifile->backup->data2 = (unsigned char *) malloc(tifile->backup->length2);
	if (tifile->backup->data2 == NULL)
		return FreeTiFile(tifile);
	for (i = 0; i < tifile->backup->length2 && !feof(infile); i++) {
		tmpread(infile);
		tifile->backup->data2[i] = tmp;
	}

	tmpread(infile);
	tifile->backup->length3a = tmp;
	tmpread(infile);
	tifile->backup->length3a += tmp << 8;

	tifile->backup->data3 = (unsigned char *) malloc(tifile->backup->length3);
	if (tifile->backup->data3 == NULL)
		return FreeTiFile(tifile);
	for (i = 0; i < tifile->backup->length3 && !feof(infile); i++) {
		tmpread(infile);
		tifile->backup->data3[i] = tmp;
	}

	tifile->chksum = (fgetc(infile) & 0xFF) + ((fgetc(infile) & 0xFF) << 8);

	tifile->type = BACKUP_TYPE;
	return tifile;
}

static int ReadIntelHex(FILE *ifile, INTELHEX_t *ihex) {
	BYTE str[600];
	char data_size[3]={0};
	char address[5]={0};
	char data_type[3]={0};
	char data_byte[3]={0};
	int index = 0;
	int i;

	memset(str, 0x00, ARRAYSIZE(str));
	if (!fgets((char*) str, 580, ifile))
		return 0;
	if (str[0] == 0)
		memcpy(str, str + 1, 579);

	if(strlen(str) < 11) return 0;

	++index;//:
	data_size[0] = str[index++];
	data_size[1] = str[index++];

	address[0] = str[index++];
	address[1] = str[index++];
	address[2] = str[index++];
	address[3] = str[index++];

	data_type[0] = str[index++];
	data_type[1] = str[index++];

	ihex->DataSize = (int) strtol(data_size, NULL, 16);
	ihex->Address = (int) strtol(address, NULL, 16);
	ihex->Type = (int) strtol(data_type, NULL, 16);

	memset(ihex->Data, 0x00, 256);

	for (i = 0; i < ihex->DataSize; ++i) {
		data_byte[0] = (str + index)[i * 2];
		data_byte[1] = (str + index)[i * 2 + 1];
		ihex->Data[i] = (BYTE) strtol(data_byte, NULL, 16);
	}

	return 1;
}













