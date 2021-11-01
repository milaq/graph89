/*
 * TilEm II
 *
 * Copyright (c) 2010-2012 Benjamin Moody
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <glib.h>
#include <tilem.h>

#include "gettext.h"

/* Memory management */

void tilem_free(void* p)
{
	g_free(p);
}

void* tilem_malloc(size_t s)
{
	return g_malloc(s);
}

void* tilem_realloc(void* p, size_t s)
{
	return g_realloc(p, s);
}

void* tilem_try_malloc(size_t s)
{
	return g_try_malloc(s);
}

void* tilem_malloc0(size_t s)
{
	return g_malloc0(s);
}

void* tilem_try_malloc0(size_t s)
{
	return g_try_malloc0(s);
}

void* tilem_malloc_atomic(size_t s)
{
	return g_malloc(s);
}

void* tilem_try_malloc_atomic(size_t s)
{
	return g_try_malloc(s);
}

/* Logging */

const char * tilem_gettext(const char *msg)
{
	return _(msg);
}

void tilem_message(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	fprintf(stderr, _("x%c: "), calc->hw.model_id);
	vfprintf(stderr, msg, ap);
	fputc('\n', stderr);
	va_end(ap);
}

void tilem_warning(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	fprintf(stderr, _("x%c: WARNING: "), calc->hw.model_id);
	vfprintf(stderr, msg, ap);
	fputc('\n', stderr);
	va_end(ap);
}

void tilem_internal(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	fprintf(stderr, _("x%c: INTERNAL ERROR: "), calc->hw.model_id);
	vfprintf(stderr, msg, ap);
	fputc('\n', stderr);
	va_end(ap);
}
