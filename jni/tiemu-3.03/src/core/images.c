/* Hey EMACS -*- linux-c -*- */
/* $Id: images.c 2821 2009-05-04 20:06:12Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Li�vin
 *  Copyright (c) 2005-2007, Romain Li�vin, Kevin Kofler
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */


/*
 * Modified to run in Android OS. Dritan Hashorva 2012
 */



/*
	This module handles loading of images or upgrades.
	Images can be:
	- ROM dump
	- FLASH upgrade as a ROM dump
	
  	Note:0x12000 is the beginning of the system privileged part.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

#include "libuae.h"

#include "intl.h"
#include "ti68k_int.h"
#include "ti68k_err.h"
#include "ti68k_def.h"
#include "images.h"
#include "hwpm.h"
#include <androidlog.h>

#define is_num(c)   isdigit(c)
#define is_alnum(c) isalnum(c)

#define SPP	0x12000		// system privileged part
#define BO  0x88        // offset from SPP to boot

IMG_INFO	img_infos;
int			img_loaded = 0;
int			img_changed = 0;


static int get_rom_version(char *ptr, int size, char *version);

/*
	Utility functions
*/
int ti68k_is_a_rom_file(const char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext == NULL)
		return 0;
	else if(!strcasecmp(ext, ".rom"))
		return !0;

	return 0;
}

int ti68k_is_a_tib_file(const char *filename)
{
	return tifiles_file_is_os(filename);
}

int ti68k_is_a_img_file(const char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext == NULL)
		return 0;
	else if(!strcasecmp(ext, ".img"))
		return !0;

	return 0;
}

int ti68k_is_a_sav_file(const char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if(ext == NULL)
		return 0;
	else if(!strcasecmp(ext, ".sav"))
		return !0;

	return 0;
}

/*
	Display information
*/
void ti68k_display_rom_infos(IMG_INFO *s)
{
	LOGI("ROM information:");
	LOGI("  Calculator  : %s", ti68k_calctype_to_string(s->calc_type));
	LOGI("  Firmware    : %s", s->version);
	LOGI("  Memory type : %s", ti68k_romtype_to_string(s->flash));
	LOGI("  Memory size : %iMB (%i bytes)", s->size >> 20, s->size);
	LOGI("  ROM base    : %02x", s->rom_base & 0xff);
	LOGI("  Hardware    : %i", s->hw_type);
}

void ti68k_display_tib_infos(IMG_INFO *s)
{
	LOGI("TIB information:");
	LOGI("  Calculator  : %s", ti68k_calctype_to_string(s->calc_type));
	LOGI("  Firmware    : %s", s->version);
	LOGI("  Memory type : %s", ti68k_romtype_to_string(s->flash));
	LOGI("  Memory size : %iMB (%i bytes)", s->size >> 20, s->size);
	LOGI("  ROM base    : %02x", s->rom_base & 0xff);
}

void ti68k_display_img_infos(IMG_INFO *s)
{
	LOGI("Image information:");
	LOGI("  Calculator  : %s", ti68k_calctype_to_string(s->calc_type));
	LOGI("  Firmware    : %s", s->version);
	LOGI("  Memory type : %s", ti68k_romtype_to_string(s->flash));
	LOGI("  Memory size : %iMB (%i bytes)", s->size >> 20, s->size);
	LOGI("  ROM base    : %02x", s->rom_base & 0xff);
    LOGI("  Hardware    : %i", s->hw_type);
    LOGI("  Has boot    : %s", s->has_boot ? "yes" : "no");
}

/*
	Get some information on the ROM dump:
	- size
	- ROM base address
	- FLASH/EPROM
	- os version
	- calc type
	Note: if the data field is NULL, memory is allocated. 
	Otherwise, data is overwritten.
	Thanks to Kevin for HW2 detection code.
*/
int ti68k_get_rom_infos(const char *filename, IMG_INFO *rom, int preload)
{
  	FILE *file;
    HW_PARM_BLOCK hwblock;

	// No filename, exits
    if(!strcmp(g_basename(filename), ""))
	    return ERR_CANT_OPEN;

	// Open file
  	file = fopen(filename, "rb");
  	if(file == NULL)
    {
      LOGI("Unable to open this file: <%s>", filename);
      return ERR_CANT_OPEN;
    }

  	// Retrieve ROM size
  	fseek(file, 0, SEEK_END);
  	rom->size = ftell(file);
  	fseek(file, 0, SEEK_SET);

  	if(rom->size < 256) 
    	return ERR_INVALID_ROM_SIZE;
	if (rom->size == 8*MB)
	{
	  // TiLP used to dump 8 MB images for HW4, try to load them anyway.
	  LOGI("Warning: truncating 8 MB image to 4 MB: <%s>", filename);
	  rom->size = 4*MB;
	}
  	if (rom->size > 4*MB)
    	return ERR_INVALID_ROM_SIZE;
  
	if(rom->data == NULL)
  		rom->data = malloc(rom->size + 4);
	if(rom->data == NULL)
		return ERR_MALLOC;
	memset(rom->data, 0xff, rom->size);
	if (fread(rom->data, 1, rom->size, file) < (size_t)rom->size)
	{
	  LOGI("Failed to read from file: <%s>", filename);
	  fclose(file);
	  return ERR_CANT_OPEN;
	}
	if (fclose(file))
	{
	  LOGI("Failed to close file: <%s>", filename);
	  return ERR_CANT_OPEN;
	}

    rom->has_boot = 1;
    rom->rom_base = rom->data[0x05] & 0xf0;
  	rom->flash = (rom->data[0x65] & 0x0f) ? 0 : FLASH_ROM;

    get_rom_version(rom->data, rom->size, rom->version);

    if(!rom->flash)
    {
        rom->calc_type = TI92;
        rom->hw_type = HW1;
    }
    else
    {
        // Get hw param block to determine calc type & hw type
        if(ti68k_get_hw_param_block((uint8_t*)rom->data, rom->rom_base, 
				    &hwblock) == -1)
	    return ERR_INVALID_ROM;
        ti68k_display_hw_param_block(&hwblock);

        switch(hwblock.hardwareID)
        {
        case HWID_TI92P: rom->calc_type = TI92p; break;
        case HWID_TI89: rom->calc_type = TI89;  break;
        case HWID_V200: rom->calc_type = V200;  break;
        case HWID_TI89T: rom->calc_type = TI89t; break;
        default: break;
        }

        if(rom->flash)
        {
            if(hwblock.len < 24)
                rom->hw_type = HW1;
            else
                rom->hw_type = (char)hwblock.gateArray;
        }
    }

	if(!preload)
		free(rom->data);

	return 0;
}


/*
  Get some information on the FLASH upgrade:
  - size
  - ROM base address
  - os version
  - calc type
*/
int ti68k_get_tib_infos(const char *filename, IMG_INFO *tib, int preload)
{
	FlashContent *content;
	FlashContent *ptr;
	int nheaders = 0;
	int i;

	// No filename, exits
	if(!strcmp(g_basename(filename), ""))
	   return ERR_CANT_OPEN;

	// Check valid file
	if(!tifiles_file_is_ti(filename))
		return ERR_NOT_TI_FILE;
		
	if(!tifiles_file_is_os(filename))
		return ERR_INVALID_UPGRADE;

	// Load file
	content = tifiles_content_create_flash(CALC_TI89);
	if(tifiles_file_read_flash(filename, content) != 0)
        return ERR_INVALID_UPGRADE;
	
	// count headers
  	for (ptr = content; ptr != NULL; ptr = ptr->next)
    	nheaders++;
  	
  	// keep the last one (data)
  	for (i = 0, ptr = content; i < nheaders - 1; i++)
    	ptr = ptr->next;
    	
  	// Load TIB into memory and relocate at SPP
	if(tib->data == NULL)
  		tib->data = malloc(SPP + ptr->data_length + 4);
	if(tib->data == NULL)
		return ERR_MALLOC;

    memset(tib->data + SPP, 0xff, ptr->data_length);
  	memcpy(tib->data + SPP, ptr->data_part, ptr->data_length);
  	
  	// Update current rom infos
    tib->rom_base = tib->data[BO+5 + SPP] & 0xf0;

	// libtifiles can't distinguish TI89/TI89t and 92+/V200. We need to look.
	switch(ptr->device_type & 0xff)
	{
		case DEVICE_TYPE_89:    // can be a Titanium, too
            switch(tib->rom_base & 0xff)
            {
            case 0x20: tib->calc_type = TI89;  break;
            case 0x80: tib->calc_type = TI89t; break;
            default: return ERR_INVALID_UPGRADE;
            }
		break;
		case DEVICE_TYPE_92P:
            switch(tib->rom_base & 0xff)
            {
            case 0x20: tib->calc_type = V200;  break;
            case 0x40: tib->calc_type = TI92p; break;
            default: return ERR_INVALID_UPGRADE;
            }
		break;
		default:
			LOGI("TIB problem: %02x!\n", 0xff & ptr->device_type);
			return ERR_INVALID_UPGRADE;
		break;
	}
    
  	tib->flash = FLASH_ROM;
  	tib->has_boot = 0;
  	tib->size = ptr->data_length + SPP;

  	get_rom_version(tib->data, tib->size, tib->version);
  	
  	tifiles_content_delete_flash(content);
	if(!preload)
		free(tib->data);

  	return 0;
}

/*
	Try to get some information on the ROM dump:
	- size
	- ROM base address
	- FLASH/EPROM
	- soft version
	- calc type
*/
int ti68k_get_img_infos(const char *filename, IMG_INFO *ri)
{
	FILE *f;
	IMG_INFO32 ri32;
	IMG_INFO64 ri64;

	// No filename, exits
	if(!strcmp(g_basename(filename), ""))
	   return ERR_CANT_OPEN;

	// Check file
	if(!ti68k_is_a_img_file(filename))
	{
		LOGW("Images must have '.img' extension (%s).\n", filename);
		return ERR_CANT_OPEN;
	}
	
	// Open dest file
  	f = fopen(filename, "rb");
  	if(f == NULL)
    {
  		LOGW("Unable to open this file: <%s>\n", filename);
      	return ERR_CANT_OPEN;
    }
    
    // Read header
    if (fread(&ri32, sizeof(IMG_INFO32), 1, f) < 1)
    {
    	LOGW("Failed to read from file: <%s>\n", filename);
      fclose(f);
      return ERR_CANT_OPEN;
    }
    *ri = ri32;

	// below is patch from Lionel
    if(strcmp(ri->signature, IMG_SIGN) || ri->size > 4*MB || ri->calc_type > CALC_MAX
       || ri->header_size == 0 || ri->hw_type > 4 || ri->rom_base == 0)
    {
      // In addition to plain invalid files, this may happen if the image was
      // created on a 64-bit platform with TIEmu <= 3.03.
      // Try to read an IMG_INFO structure as it used to be written by those
      // 64-bit platforms.
      fseek(f, 0, SEEK_SET);
      if (fread(&ri64, sizeof(IMG_INFO64), 1, f) < 1)
      {
        LOGW("Failed to read from file: <%s>\n", filename);
        fclose(f);
        return ERR_CANT_OPEN;
      }
      else {
        memcpy(ri->signature, &(ri64.signature), sizeof(ri64.signature));
        ri->revision = (int32_t)(ri64.revision);
        ri->header_size = (int32_t)(ri64.header_size);

        ri->calc_type = ri64.calc_type;
        memcpy(ri->version, &(ri64.version), sizeof(ri64.version));
        ri->flash = ri64.flash;
        ri->has_boot = ri64.has_boot;
        ri->size = (int32_t)(ri64.size);
        ri->hw_type = ri64.hw_type;
        ri->rom_base = ri64.rom_base;
          
        if(strcmp(ri->signature, IMG_SIGN) || ri->size > 4*MB || ri->calc_type > CALC_MAX
           || ri->header_size == 0 || ri->hw_type > 4 || ri->rom_base == 0)
        {
          // Nope, it still doesn't seem to be a TIEmu image.
          LOGW("Bad image: <%s>\n", filename);
          return ERR_INVALID_UPGRADE;
        }
        else {
          LOGI("Found a reasonably valid 64-bit IMG_INFO in <%s>\n", filename);
        }
      }
    }

    // Close file
    if (fclose(f))
    {
      LOGW("Failed to close file: <%s>\n", filename);
      return ERR_CANT_OPEN;
    }
    
    return 0;
}

/*
  	Convert a romdump into an image.
	This kind of image is complete (boot & certificate).
*/
int ti68k_convert_rom_to_image(const char *srcname, const char *dest,  int* calc_type)
{
  	FILE *f; 
  	int err;
	IMG_INFO img;

	// No filename, exits
	if(!strcmp(g_basename(srcname), ""))
	   return ERR_CANT_OPEN;

	// Preload romdump
	memset(&img, 0, sizeof(IMG_INFO));
	err = ti68k_get_rom_infos(srcname, &img, !0);
	if(err)
    {
	    free(img.data);
      	LOGI("Unable to get information on ROM dump: %s", srcname);
      	return err;
    }
	ti68k_display_rom_infos(&img);

	// Open dest file
  	f = fopen(dest, "wb");
  	if(f == NULL)
    {
      	LOGW("Unable to open this file: <%s>\n", dest);
      	return ERR_CANT_OPEN;
    }

	// Some V200 and TI89 Titanium ROMs are half the size
	if((img.size < 4*MB) && (img.calc_type == V200 || img.calc_type == TI89t))
	{
		img.size = 4*MB;
		img.data = realloc(img.data, 4*MB + 4);
		LOGI("Completing image to 4 MB!");
		memset(img.data + 2*MB, 0xff, 2*MB);
	}

	// Fill header
	strcpy(img.signature, IMG_SIGN);
	img.header_size = sizeof(IMG_INFO);
    img.revision = IMG_REV;

	// Write file
	if (fwrite(&img, 1, sizeof(IMG_INFO), f) < (size_t)sizeof(IMG_INFO)
	    || fwrite(img.data, sizeof(char), img.size, f) < (size_t)img.size)
	{
	  LOGW("Failed to write to file: <%s>\n", dest);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}

	// Close file
	if (fclose(f))
	{
	  LOGW("Failed to close file: <%s>\n", dest);
	  return ERR_CANT_OPEN;
	}

	*calc_type = img.calc_type;

	return 0;
}

/*
	Convert an upgrade into an image.
  	The image has neither boot block nor certificate.
*/
int ti68k_convert_tib_to_image(const char *srcname, const char *dest, int hw_type, int* calc_type)
{
	FILE *f; 
  	int err;
	IMG_INFO img;
	int i, j;
	int num_blocks, last_block;
    	int real_size;
	HW_PARM_BLOCK hwpb;

	// No filename, exits
	if(!strcmp(g_basename(srcname), ""))
	   return ERR_CANT_OPEN;

	// Preload upgrade
	memset(&img, 0, sizeof(IMG_INFO));
	err = ti68k_get_tib_infos(srcname, &img, !0);
	if(err)
    {
	    free(img.data);
      	LOGI("Unable to get information on FLASH upgrade: <%s>", srcname);
      	return err;
    }
	ti68k_display_tib_infos(&img);

	// Open dest file
  	f = fopen(dest, "wb");
  	if(f == NULL)
    {
      	LOGW("Unable to open this file: <%s>\n", dest);
      	return ERR_CANT_OPEN;
    }

	// Fill header
	strcpy(img.signature, IMG_SIGN);
	img.header_size = sizeof(IMG_INFO);
	img.revision = IMG_REV;
    real_size = img.size - SPP;
    img.size = ti68k_get_rom_size(img.calc_type);

    img.hw_type = hw_type;
	if(hw_type == -1)
	{
		if(img.calc_type == TI89t)
			img.hw_type = HW3;  //default
		else if(img.calc_type == TI89 || img.calc_type == TI92p || img.calc_type == V200)
			img.hw_type = HW2;	// default
	}
	
	// Write header
	if (fwrite(&img, 1, sizeof(IMG_INFO), f) < sizeof(IMG_INFO))
	{
	  LOGW("Failed to write to file: <%s>\n", dest);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}

	// Write boot block
	memcpy(img.data, &img.data[SPP + BO], 256);
	if (fwrite(img.data, 1, 256, f) < 256)
	{
	  LOGW("Failed to write to file: <%s>\n", dest);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}

    // Write hardware param block

	// fill structure
	hwpb.len = 24;
	switch(img.calc_type)
	{
		case TI89:
			hwpb.hardwareID = HWID_TI89;
			hwpb.hardwareRevision = img.hw_type - 1;
			break;
		case TI92p:
			hwpb.hardwareID = HWID_TI92P;
			hwpb.hardwareRevision = img.hw_type - 1;
			break;
		case V200:
			hwpb.hardwareID = HWID_V200;
			hwpb.hardwareRevision = 2;
			break;
		case TI89t:
			hwpb.hardwareID = HWID_TI89T;
			hwpb.hardwareRevision = 2;
			break;
	}
	hwpb.bootMajor = hwpb.bootRevision = hwpb.bootBuild = 1;
	hwpb.gateArray = img.hw_type;
	ti68k_put_hw_param_block((uint8_t *)img.data, img.rom_base, &hwpb);

	// write filler
	if (fputc(0xfe, f) < 0 || fputc(0xed, f) < 0 || fputc(0xba, f) < 0 || fputc(0xbe, f) < 0
	//fwrite(&hwpb, 1hwpb.len+2, f);

	// write address (pointer)
	|| fputc(0x00, f) < 0
	|| fputc(img.rom_base, f) < 0
	|| fputc(0x01, f) < 0
	|| fputc(0x08, f) < 0

	// write structure
	|| fputc(MSB(hwpb.len), f) < 0
	|| fputc(LSB(hwpb.len), f) < 0
	|| fputc(MSB(MSW(hwpb.hardwareID)), f) < 0
	|| fputc(LSB(MSW(hwpb.hardwareID)), f) < 0
	|| fputc(MSB(LSW(hwpb.hardwareID)), f) < 0
	|| fputc(LSB(LSW(hwpb.hardwareID)), f) < 0
	|| fputc(MSB(MSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(LSB(MSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(MSB(LSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(LSB(LSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(MSB(MSW(hwpb.bootMajor)), f) < 0
	|| fputc(LSB(MSW(hwpb.bootMajor)), f) < 0
	|| fputc(MSB(LSW(hwpb.bootMajor)), f) < 0
	|| fputc(LSB(LSW(hwpb.bootMajor)), f) < 0
	|| fputc(MSB(MSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(LSB(MSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(MSB(LSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(LSB(LSW(hwpb.hardwareRevision)), f) < 0
	|| fputc(MSB(MSW(hwpb.bootBuild)), f) < 0
	|| fputc(LSB(MSW(hwpb.bootBuild)), f) < 0
	|| fputc(MSB(LSW(hwpb.bootBuild)), f) < 0
	|| fputc(LSB(LSW(hwpb.bootBuild)), f) < 0
	|| fputc(MSB(MSW(hwpb.gateArray)), f) < 0
	|| fputc(LSB(MSW(hwpb.gateArray)), f) < 0
	|| fputc(MSB(LSW(hwpb.gateArray)), f) < 0
	|| fputc(LSB(LSW(hwpb.gateArray)), f) < 0)
	{
	  LOGW("Failed to write to file: <%s>\n", dest);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}

	// Fill with 0xff up-to System Part*
	for(i = 0x108 + hwpb.len+2; i < SPP; i++)
		if (fputc(0xff, f) < 0)
		{
		  LOGW("Failed to write to file: <%s>\n", dest);
		  fclose(f);
		  return ERR_CANT_OPEN;
		}
 
	// Copy FLASH upgrade at 0x12000 (SPP)
	num_blocks = real_size / 65536;
	for(i = 0; i < num_blocks; i++ )
	{
		LOGI(".");
		fflush(stdout);

		if (fwrite(&img.data[65536 * i + SPP], sizeof(char), 65536, f) < 65536)
		{
		  LOGW("Failed to write to file: <%s>\n", dest);
		  fclose(f);
		  return ERR_CANT_OPEN;
		}
	}

	last_block = real_size % 65536;
	if (fwrite(&img.data[65536 * i + SPP], sizeof(char), last_block, f) < (size_t)last_block)
	{
	  LOGW("Failed to write to file: <%s>\n", dest);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}
 
	LOGI("");
	LOGI("Completing to %iMB size\n", img.size >> 20);
	for(j = SPP + real_size; j < img.size; j++)
		if (fputc(0xff, f) < 0)
		{
		  LOGW("Failed to write to file: <%s>\n", dest);
		  fclose(f);
		  return ERR_CANT_OPEN;
		}
 
	// Close file
	if (fclose(f))
	{
	  LOGW("Failed to close file: <%s>\n", dest);
	  return ERR_CANT_OPEN;
	}

	*calc_type = img.calc_type;
	return 0;
}

/*
    Convert an romdump into image and replace SPP by upgrade.
    The resulting image has boot block.
*/
int ti68k_merge_rom_and_tib_to_image(const char *srcname1, const char *srcname2,
                                     const char *dirname, char **dstname)
{
    FILE *f; 
  	int err;
	IMG_INFO img;
	char *ext;
	gchar *basename;
    int real_size;

    *dstname = NULL;

	// No filename, exits
	if(!strcmp(g_basename(srcname1), ""))
		return ERR_CANT_OPEN;

	if(!strcmp(g_basename(srcname2), ""))
		return ERR_CANT_OPEN;

	// Preload romdump
    memset(&img, 0, sizeof(IMG_INFO));
	err = ti68k_get_rom_infos(srcname1, &img, !0);
	if(err)
    {
	    free(img.data);
      	LOGI("Unable to get information on ROM dump: %s", srcname1);
      	return err;
    }
	ti68k_display_rom_infos(&img);

    // Save size
    real_size = img.size;

    // Load upgrade

    err = ti68k_get_tib_infos(srcname2, &img, !0);
	if(err)
    {
	    free(img.data);
      	LOGI("Unable to get information on ROM dump: %s", srcname2);
      	return err;
    }
	ti68k_display_tib_infos(&img);

	// Create destination file
	basename = g_path_get_basename(srcname1);
	ext = strrchr(basename, '.');
  	*ext='\0';
	strcat(basename, ".img");

	*dstname = g_strconcat(dirname, basename, NULL);
	g_free(basename);

    // Restore size
    img.size = real_size;

	// Open dest file
	f = fopen(*dstname, "wb");
	if(f == NULL)
	{
		LOGW("Unable to open this file: <%s>\n", *dstname);
		return ERR_CANT_OPEN;
	}

	// Fill header
	strcpy(img.signature, IMG_SIGN);
	img.header_size = sizeof(IMG_INFO);
	img.revision = IMG_REV;
	img.has_boot = 1;

	// Write file
	if (fwrite(&img, 1, sizeof(IMG_INFO), f) < sizeof(IMG_INFO)
	    || fwrite(img.data, sizeof(char), img.size, f) < (size_t)img.size)
	{
	  LOGW("Failed to write to file: <%s>\n", *dstname);
	  fclose(f);
	  return ERR_CANT_OPEN;
	}

	// Close file
	if (fclose(f))
	{
	  LOGW("Failed to close file: <%s>\n", *dstname);
	  return ERR_CANT_OPEN;
	}

	return 0;
}


/*
  	This function loads an image.
*/
int ti68k_load_image(const char *filename)
{
	IMG_INFO *img = &img_infos;
  	FILE *f;  	
  	int err;

	// Clear infos
	memset(img, 0, sizeof(IMG_INFO));

	// No filename, exits
	if(!strcmp(g_basename(filename), ""))
	   return ERR_CANT_OPEN;

	// Load infos
	err = ti68k_get_img_infos(filename, img);
  	if(err)
    {
      	LOGI("Unable to get information on image: %s", filename);
      	return err;
    }
	ti68k_display_img_infos(img);
	
	// Open file
	f = fopen(filename, "rb");
	if(f == NULL)
	{
		LOGW("Unable to open this file: <%s>\n", filename);
		return ERR_CANT_OPEN;
	}

	// Read pure data
	if (fseek(f, img->header_size, SEEK_SET))
	{
		LOGW("Failed to read from file: <%s>\n", filename);
		fclose(f);
		return ERR_CANT_OPEN;
	}

	img->data = malloc(img->size + 4);
	if(img->data == NULL)
		return ERR_MALLOC;
	if (fread(img->data, 1, img->size, f) < (size_t)img->size)
	{
		LOGW("Failed to read from file: <%s>\n", filename);
		fclose(f);
		return ERR_CANT_OPEN;
	}

#if 1
	{
		HW_PARM_BLOCK hwblock;

		ti68k_get_hw_param_block((uint8_t *)img->data, img->rom_base, 
					 &hwblock);
		ti68k_display_hw_param_block(&hwblock);
	}
#endif

	if (fclose(f))
	{
		LOGW("Failed to close file: <%s>\n", filename);
		return ERR_CANT_OPEN;
	}

	img_loaded = 1;
	img_changed = 1;

	return 0;
}

/*
  	Load a FLASH upgrade (.tib/.9xu/.89u).
  	Note: an image must have been loaded before 
  	calling this function.
*/
int ti68k_load_upgrade(const char *filename)
{
	IMG_INFO tib;
  	int err;
    IMG_INFO *img = &img_infos;

	if(!img_loaded)
		return -1;

	// No filename, exits
	if(!strcmp(g_basename(filename), ""))
		return 0;	//ERR_CANT_OPEN;

	memset(&tib, 0, sizeof(IMG_INFO));
	err = ti68k_get_tib_infos(filename, &tib, !0);
	if(err)
    {
		free(img->data);
      	LOGI("Unable to get information on FLASH upgrade: <%s>", filename);
      	return err;
    }
	ti68k_display_tib_infos(&tib);

    // Allow upgrade ?
    if(tib.calc_type != img->calc_type)
    {
        free(tib.data);
        return ERR_CANT_UPGRADE;
    }

	tib.has_boot = 1;	// still bootable
	memset(tihw.rom+SPP, 0xff, tihw.rom_size-SPP);	// clear FLASH
	memcpy(tihw.rom+SPP, tib.data+SPP, tib.size-SPP);
    free(tib.data);

	strcpy(tihw.rom_version, tib.version);

  	img_loaded = 2;
	img_changed = 2;

	return 0;
}

/*
  	Unload an image (free memory).
*/
int ti68k_unload_image_or_upgrade(void)
{
	IMG_INFO *img = &img_infos;

	if(!img_loaded)
		return -1;

	img->data = NULL;
	img_loaded = 0;

	return 0;
}


/*
    Search for ROM dumps or FLASH upgrades in a given directory 
	and converts them into images (note: original file is deleted !).
*/
int ti68k_scan_files(const char *src_dir, const char *dst_dir, int erase)
{
/*    GDir *dir;
	GError *error = NULL;
	G_CONST_RETURN gchar *dirent;
    gchar *path;
    int ret;
    gchar *dstname;

    // Search for  files and convert them
	dir = g_dir_open(src_dir, 0, &error);
	if (dir == NULL) 
	{
		LOGW("Opendir error");
      	return ERR_CANT_OPEN_DIR;
	}

    while ((dirent = g_dir_read_name(dir)) != NULL) 
	{
  		if (dirent[0] == '.') 
  			continue;

        path = g_strconcat(src_dir, dirent, NULL);

        if(ti68k_is_a_rom_file(path))
        {
            ret = ti68k_convert_rom_to_image(path, dst_dir, &dstname);
			if(ret)
				{
					g_free(dstname);
					g_free(path);
					return ret;
						}

            if(erase)
                unlink(path);

            g_free(dstname);
        }

		if(ti68k_is_a_tib_file(path))
        {
            ret = ti68k_convert_tib_to_image(path, dst_dir, &dstname, -1);
			if(ret)
				{
					g_free(dstname);
					g_free(path);
				return ret;
					}

            if(erase)
                unlink(path);

            g_free(dstname);
        }

        g_free(path);
    }

    g_dir_close(dir);
*/
    return 0;
}

/*
  	Scan images in a given directory and write list into img_list.txt.
*/
int ti68k_scan_images(const char *dirname, const char *filename)
{
	FILE *file;
	IMG_INFO img;
	GDir *dir;
	GError *error = NULL;
	G_CONST_RETURN gchar *dirent;
	gchar *path, *str;
	int ret;
	struct stat f_info;
  	char *line[7];

  	LOGI("Scanning images/upgrades... ");

	// Create file (and overwrite)
	file = fopen(filename, "wt");
    if(file == NULL)
	{
	  	LOGW("Unable to open this file: <%s>", filename);
	  	return ERR_CANT_OPEN;
	} 	

  	// List all files available in the directory
	dir = g_dir_open(dirname, 0, &error);
	if (dir == NULL) 
	{
		LOGW("Opendir error");
      	return ERR_CANT_OPEN_DIR;
	}
  
	while ((dirent = g_dir_read_name(dir)) != NULL) 
	{
  		if (dirent[0] == '.') 
  			continue;
   
	  	path = g_strconcat(dirname, dirent, NULL);
	  	
		ret = stat(path, &f_info);
		if(ret == -1)
		{
			LOGW("Can not stat: <%s>", dirent);
	      	perror("stat: ");
		}
		else
		{
			if(ti68k_is_a_img_file(path))
			{
				memset(&img, 0, sizeof(IMG_INFO));
				ret = ti68k_get_img_infos(path, &img);
				if(ret)
				{
					LOGW("Can not get ROM/update info: <%s>", path);
					break;
				}
			}
            else
				continue;

			str = g_strdup_printf("%iKB", (int)(img.size >> 10));

		  	line[0] = (char *)dirent;
		  	line[1] = (char *)ti68k_calctype_to_string(img.calc_type);
	  		line[2] = img.version;
	  		line[3] = (char *)ti68k_romtype_to_string(img.flash);
	  		line[4] = str;
			line[5] = img.has_boot ? _("yes") : _("no");
			line[6] = (char *)ti68k_hwtype_to_string(img.hw_type);
	  
		  	fprintf(file, "%s,%s,%s,%s,%s,%s,%s\n", 
		  			line[0], line[1], line[2], 
		  			line[3], line[4], line[5], line[6]);
			g_free(str);
		}
	  	g_free(path);
    }      

	// Close
	g_dir_close(dir);
  
  	fclose(file);
  	LOGI("Done.");
  
  	return 0;
}


/*
  	Search the version string in the ROM
	Arguments:
  	- ptr: a ROM or update image
  	- size: the size of the buffer
  	- version: the returned string version
*/
static int get_rom_version(char *ptr, int size, char *version)
{
  	int i;

  	strcpy(version, "?.??");

  	for (i = SPP; i < size-16; i += 2)
    {
      if (is_num(ptr[i])&&(ptr[i+1]=='.') && is_num(ptr[i+2]) &&
	  (ptr[i+3]==0)&&is_alnum(ptr[i+4]) && is_alnum(ptr[i+5]) &&
	  (ptr[i+6]=='/')&&is_alnum(ptr[i+7]) && is_alnum(ptr[i+8]) &&
	  (ptr[i+9]=='/')&&is_alnum(ptr[i+10]) && is_alnum(ptr[i+11]))
	  	break;

      if (is_num(ptr[i]) && (ptr[i+1]=='.') && is_num(ptr[i+2]) && 
	  is_num(ptr[i+3]) && (ptr[i+4]==0) && is_alnum(ptr[i+5]) && 
	  is_alnum(ptr[i+6]) && (ptr[i+7]=='/') && is_alnum(ptr[i+8]) && 
	  is_alnum(ptr[i+9]) && (ptr[i+10]=='/') && is_alnum(ptr[i+11]) && 
	  is_alnum(ptr[i+12]))
		break;
	
	  if (is_num(ptr[i]) && (ptr[i+1]=='.') && is_num(ptr[i+2]) && 
	  (ptr[i+3]==0) && is_alnum(ptr[i+4]) && is_alnum(ptr[i+5]) && 
	  is_alnum(ptr[i+6]) && is_alnum(ptr[i+7]) && is_alnum(ptr[i+8]) && 
	  is_alnum(ptr[i+9]) && is_alnum(ptr[i+10]) && is_alnum(ptr[i+11]))
	  	break;

      if (is_num(ptr[i]) && (ptr[i+1]=='.') && is_num(ptr[i+2]) && 
	  is_alnum(ptr[i+3]) && (ptr[i+4]==0) && is_alnum(ptr[i+5]) && 
	  is_alnum(ptr[i+6]) && is_alnum(ptr[i+7]) && is_alnum(ptr[i+8]) && 
	  is_alnum(ptr[i+9]) && is_alnum(ptr[i+10]) && is_alnum(ptr[i+11]))
	  	break;
    }
  
  	if (i < size-16) 
    {
      	int n;
      
      	for(n = i; n < i+16; n++) 
		{
	  		if (ptr[n]==0) 
	    	{
	      		strcpy(version, ptr+i);
	      		(version)[n-i]=0;

	      		return 0;
	    	}
		}
    }
    
  	return 0;
}

/*
    Returns the first found image
*/
int ti68k_find_image(const char *dirname, char **dst_name)
{
    GDir *dir;
	GError *error = NULL;
	G_CONST_RETURN gchar *dirent;
    int ret = 0;
    char *filename;
    
    if(dst_name != NULL)
	    *dst_name = NULL;

    // Search for *.img files and convert them
	dir = g_dir_open(dirname, 0, &error);
	if (dir == NULL) 
	{
		LOGW("Opendir error");
      	return ERR_CANT_OPEN_DIR;
	}

    filename = NULL;

    while ((dirent = g_dir_read_name(dir)) != NULL) 
	{
  		if (dirent[0] == '.') 
  			continue;

        if(!ti68k_is_a_img_file(dirent))
            continue;

        filename = g_strconcat(dirname, dirent, NULL);
        ret = !0;
        break;
    }

    g_dir_close(dir);

    if(dst_name != NULL)
        *dst_name = filename;

    return ret;
}
