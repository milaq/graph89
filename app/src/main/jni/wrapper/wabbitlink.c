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
#include <wabbit.h>
#include <tilem.h>
#include <stdlib.h>

#define FLASH_PAGES(calc)  ((calc)->hw.romsize / 0x4000)
#define FALSE 0
#define TRUE !FALSE

void state_userpages(TilemCalc* calc, upages_t *upages) {
	switch (calc->hw.model_id) {
	case TILEM_CALC_TI73:
		upages->start = TI_73_APPPAGE;
		upages->end = upages->start - TI_73_USERPAGES;
		break;
	case TILEM_CALC_TI83P:
		upages->start = TI_83P_APPPAGE;
		upages->end = upages->start - TI_83P_USERPAGES;
		break;
	case TILEM_CALC_TI83P_SE:
		upages->start = TI_83PSE_APPPAGE;
		upages->end = upages->start - TI_83PSE_USERPAGES;
		break;
	case TILEM_CALC_TI84P:
		upages->start = TI_84P_APPPAGE;
		upages->end = upages->start - TI_84P_USERPAGES;
		break;
	case TILEM_CALC_TI84P_SE:
		upages->start = TI_84PSE_APPPAGE;
		upages->end = upages->start - TI_84PSE_USERPAGES;
		break;
	default:
		upages->start = -1;
		upages->end = 0;
		break;
	}
}

int find_header(u_char (*dest)[PAGE_SIZE_W], int page, int ident1, int ident2) {
	int i;
	//apparently non user apps have a slightly different header
	//therefore we have to actually find the identifier
	for (i = 0; i < PAGE_SIZE_W; i++)
		if (dest[page][i] == ident1 && dest[page][i + 1] == ident2)
			return dest[page][i + 2];
	return -1;
}

BOOL check_flashpage_empty(u_char (*dest)[PAGE_SIZE_W], u_int page,
		u_int num_pages) {
	u_char *space = &dest[page][PAGE_SIZE_W - 1];
	u_int i;
	// Make sure the subsequent pages are empty
	for (i = 0; i < num_pages * PAGE_SIZE_W; i++, space--) {
		if (*space != 0xFF) {
			printf("Subsequent pages not empty\n");
			return FALSE;
		}
	}
	return TRUE;
}

void state_build_applist(TilemCalc* calc, applist_t *applist) {
	applist->count = 0;

	if (calc->mem == NULL)
		return;
	u_char (*flash)[PAGE_SIZE_W] = (u_char (*)[PAGE_SIZE_W]) calc->mem;

	// fetch the userpages for this model
	upages_t upages;
	state_userpages(calc, &upages);
	if (upages.start == -1)
		return;

	// Starting at the first userpage, search for all the apps
	// As soon as page doesn't have one, you're done
	u_int page, page_size;
	for (page = upages.start, applist->count = 0;
			page >= upages.end && applist->count < ARRAYSIZE(applist->apps)
					&& flash[page][0x00] == 0x80 && flash[page][0x01] == 0x0F
					&& find_header(flash, page, 0x80, 0x48) != -1
					&& (page_size = find_header(flash, page, 0x80, 0x81)) != -1;

			page -= page_size, applist->count++) {

		apphdr_t *ah = &applist->apps[applist->count];
		u_int i;
		for (i = 0; i < PAGE_SIZE_W; i++)
			if (flash[page][i] == 0x80 && flash[page][i + 1] == 0x48)
				break;
		memcpy(ah->name, &flash[page][i + 2], 8);
		ah->name[8] = '\0';
		ah->page = page;
		ah->page_count = find_header(flash, page, 0x80, 0x81);

	}
}

/* Fixes the certificate page so that the app is no longer marked as a trial
 * cpu: cpu for the core the application is on
 * page: the page the application you want to mark is on
 */
void fix_certificate(TilemCalc* calc, u_int page) {

	u_char (*dest)[PAGE_SIZE_W] = (u_char (*)[PAGE_SIZE_W]) calc->mem;
	upages_t upages;
	state_userpages(calc, &upages);
	//there is probably some logic here that I'm missing...
	//the 83p wtf is up with that offset
	int offset = 0x1E50;
	if (calc->hw.model_id == TILEM_CALC_TI83P)
		offset = 0x1F18;
	//erase the part of the certificate that marks it as a trial app
	dest[FLASH_PAGES(calc) - 2][offset + 2 * (upages.start - page)] = 0x80;
	dest[FLASH_PAGES(calc) - 2][offset + 1 + 2 * (upages.start - page)] = 0x00;
}

int get_page_size(u_char (*dest)[PAGE_SIZE_W], unsigned int page) {
	int i;
	//apparently non user apps have a slightly different header
	//therefore we have to actually find the identifier
	for (i = 0; i < PAGE_SIZE_W; i++)
		if (dest[page][i] == 0x80 && dest[page][i + 1] == 0x81)
			break;
	i += 2;
	return dest[page][i];
}

LINK_ERR forceload_app(TilemCalc* calc, TIFILE_t *tifile) {
	u_char (*dest)[PAGE_SIZE_W] = (u_char (*)[PAGE_SIZE_W]) calc->mem;
	if (dest == NULL)
		return LERR_MODEL;

	if (tifile->flash == NULL)
		return LERR_FILE;

	int upper_flash_index = -1;

	int i;

	upages_t upages;
	state_userpages(calc, &upages);
	if (upages.start == -1)
		return LERR_MODEL;

	for (i = 0; i < calc->hw.nhwregs; i++) {
		if (!strcmp("port23", calc->hw.hwregnames[i])) {
			upper_flash_index = i;
			break;
		}
	}

	if (upper_flash_index < 0)
	{
		return LERR_MODEL;
	}

	u_int page;
	for (page = upages.start;
			page >= upages.end + tifile->flash->pages
					&& dest[page][0x00] == 0x80 && dest[page][0x01] == 0x0F;) {
		int page_size;
		//different size app need to send the long way
		if (!memcmp(&dest[page][0x12], &tifile->flash->data[0][0x12], 8)) {
			if (get_page_size(dest, page) != tifile->flash->pages) {
				//or we can force load it still ;D
				//there's probably some good reason Jim didn't write this code :|
				int pageDiff = tifile->flash->pages - get_page_size(dest, page);
				u_int currentPage = page - tifile->flash->pages;
				u_int end_page =
						pageDiff > 0 ? currentPage : currentPage + pageDiff;
				while (!check_flashpage_empty(dest, end_page, 1)
						&& end_page >= upages.end)
					end_page -= get_page_size(dest, end_page);
				if (end_page != currentPage) {
					if (pageDiff > 0) {
						if (end_page - pageDiff < upages.end)
							return LERR_MEM;
						memmove(dest[currentPage - pageDiff], dest[currentPage],
								PAGE_SIZE_W * (end_page - currentPage));
						/*	if (cpu->pio.model == TI_83P) {
						 //mark pages unprotected
						 for (i = end_page - 7; i <= end_page + pageDiff - 8; i++) {
						 cpu->mem_c->protected_page[i / 8] &= ~(1 << (i % 8));
						 }
						 }*/
					} else {
						//0xFF all extra pages
						for (i = tifile->flash->pages;
								i < tifile->flash->pages - pageDiff;
								i++, currentPage--) {
							memset(dest[currentPage], 0xFF, PAGE_SIZE_W);
						}
						/*
						 if (cpu->pio.model == TI_83P) {
						 //mark pages as protected
						 for (i = end_page - 7; i <= end_page - pageDiff - 8; i++) {
						 cpu->mem_c->protected_page[i / 8] |= 1 << (i % 8);
						 }
						 }*/
					}
				}
				//fix page execution permissions

				if (upper_flash_index >= 0)
					calc->hwregs[upper_flash_index] -= pageDiff;
				//cpu->mem_c->flash_upper -= pageDiff;
			}
			u_int i;
			for (i = 0; i < tifile->flash->pages; i++, page--) {
				memcpy(dest[page], tifile->flash->data[i], PAGE_SIZE_W);
			}
			//note that this does not fix the old marks, only ensures that
			//the new order of apps has the correct parts marked
			applist_t applist;
			state_build_applist(calc, &applist);
			for (i = 0; i < applist.count; i++) {
				fix_certificate(calc, applist.apps[i].page);
			}

			printf("Found already\n");
			return LERR_SUCCESS;
		}
		page_size = get_page_size(dest, page);
		page -= page_size;
	}

	if (page - tifile->flash->pages < upages.end)
		return LERR_MEM;

	//mark the app as non trial
	fix_certificate(calc, page);
	//force reset the app list says BrandonW. seems to work, apps show up (sometimes)
	//mem_write(calc->mem, 0x9C87, 0x00);
	(*calc->hw.z80_wrmem)(calc, 0x9C87, 0x00);

	//u_char *space = &dest[page][PAGE_SIZE - 1];
	// Make sure the subsequent pages are empty
	if (!check_flashpage_empty(dest, page, tifile->flash->pages))
		return LERR_MEM;
	for (i = 0; i < tifile->flash->pages; i++, page--) {
		memcpy(dest[page], tifile->flash->data[i], PAGE_SIZE_W);
	}

	if (upper_flash_index >= 0)
		calc->hwregs[upper_flash_index] -= tifile->flash->pages;

	/*
	 for (i = page - 7; i <= page + tifile->flash->pages - 8; i++) {
	 //-8 is for the start of user mem
	 cpu->mem_c->protected_page[i / 8] &= ~(1 << (i % 8));
	 }*/

	return LERR_SUCCESS;
}

void calc_erase_certificate(unsigned char *mem, int size) {
	if (mem == NULL || size < 32768)
		return;

	memset(mem + size - 32768, 0xFF, PAGE_SIZE_W);

	mem[size - 32768] = 0x00;
	mem[size - 32768 + 0x1FE0] = 0x00;
	mem[size - 32768 + 0x1FE1] = 0x00;
	return;
}

LINK_ERR forceload_os(TilemCalc* calc, TIFILE_t *tifile) {
	int flash_pages = FLASH_PAGES(calc);

	u_int i, page;
	u_char (*dest)[PAGE_SIZE_W] = (u_char (*)[PAGE_SIZE_W]) calc->mem;
	if (dest == NULL)
		return LERR_MODEL;

	if (tifile->flash == NULL)
		return LERR_FILE;

	for (i = 0; i < ARRAYSIZE(tifile->flash->data); i++) {
		if (tifile->flash->data[i] == NULL) {
			continue;
		}
		if (i > 0x10) {
			page = i + flash_pages - 0x20;
		} else {
			page = i;
		}
		int sector = (page / 4) * 4;
		int size;
		if (sector >= flash_pages - 4) {
			size = PAGE_SIZE_W * 2;
		} else {
			size = PAGE_SIZE_W * 4;
		}
		memset(dest[sector], 0xFF, size);
	}
	for (i = 0; i < ARRAYSIZE(tifile->flash->data); i++) {
		if (tifile->flash->data[i] == NULL) {
			continue;
		}
		if (i > 0x10) {
			page = i + flash_pages - 0x20;
		} else {
			page = i;
		}

		memcpy(dest[page], tifile->flash->data[i], PAGE_SIZE_W);
	}

	//valid OS
	dest[0][0x56] = 0x5A;
	dest[0][0x57] = 0xA5;

	return LERR_SUCCESS;
}

LINK_ERR link_send_var(TilemCalc* calc, TIFILE_t *tifile, SEND_FLAG dest) {

	if (tifile->type == FLASH_TYPE) {
		switch (tifile->flash->type) {
		case FLASH_TYPE_OS:
			return forceload_os(calc, tifile);
		case FLASH_TYPE_APP:
			return forceload_app(calc, tifile);
		}
	}

	return LERR_SUCCESS;
}
