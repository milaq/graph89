/* Hey EMACS -*- linux-c -*- */
/* $Id: tsource.c 2268 2006-11-06 17:18:51Z roms $ */

/*  TiEmu - Tiemu Is an EMUlator
 *
 *  Copyright (c) 2000-2001, Thomas Corvazier, Romain Lievin
 *  Copyright (c) 2001-2003, Romain Lievin
 *  Copyright (c) 2003, Julien Blache
 *  Copyright (c) 2004, Romain Liévin
 *  Copyright (c) 2005, Romain Liévin
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
	Custom g_timeout_xxx support.

	Timeout support provided by GLib is unusable for our job because:
	- we could start a new timeout from callback but this doesn't work
	because timeout finalizing wan be done at _any_ time,
	- if we want to modify timeout value on the fly, we can't,
	- it does not try to 'catch up' time lost in delays.

	Solution used: I have re-implemented GLib timeout but this version
	catch up time lost in delays (time spend in callback).

	Widely taken from gmain.c of GLib.
*/

#ifdef HAVE_CONFIG_H
#include <tiemuconfig.h>
#endif

#include <glib.h>

typedef struct
{
  GSource     source;
  GTimeVal    expiration;
  guint       interval;
} GTimeoutSource;

static void
g_timeout_set_expiration (GTimeoutSource *timeout_source,
						  GTimeVal       *current_time)
{
  guint seconds = timeout_source->interval / 1000;
  guint msecs = timeout_source->interval - seconds * 1000;

  timeout_source->expiration.tv_sec = current_time->tv_sec + seconds;
  timeout_source->expiration.tv_usec = current_time->tv_usec + msecs * 1000;
  if (timeout_source->expiration.tv_usec >= 1000000)
    {
      timeout_source->expiration.tv_usec -= 1000000;
      timeout_source->expiration.tv_sec++;
    }
}

static gboolean
g_timeout_prepare  (GSource  *source,
		    gint     *timeout)
{
  glong sec;
  glong msec;
  GTimeVal current_time;
  
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  g_source_get_current_time (source, &current_time);

  sec = timeout_source->expiration.tv_sec - current_time.tv_sec;
  msec = (timeout_source->expiration.tv_usec - current_time.tv_usec) / 1000;

  /* We do the following in a rather convoluted fashion to deal with
   * the fact that we don't have an integral type big enough to hold
   * the difference of two timevals in millseconds.
   */
  if (sec < 0 || (sec == 0 && msec < 0))
    msec = 0;
  else
    {
      glong interval_sec = timeout_source->interval / 1000;
      glong interval_msec = timeout_source->interval % 1000;

      if (msec < 0)
	{
	  msec += 1000;
	  sec -= 1;
	}
      
      if (sec > interval_sec ||
	  (sec == interval_sec && msec > interval_msec))
	{
	  /* The system time has been set backwards, so we
	   * reset the expiration time to now + timeout_source->interval;
	   * this at least avoids hanging for long periods of time.
	   */
	  g_timeout_set_expiration (timeout_source, &current_time);
	  msec = MIN (G_MAXINT, timeout_source->interval);
	}
      else
	{
	  msec = MIN (G_MAXINT, (guint)msec + 1000 * (guint)sec);
	}
    }

  *timeout = (gint)msec;
  
  return msec == 0;
}

static gboolean 
g_timeout_check (GSource  *source)
{
  GTimeVal current_time;
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  g_source_get_current_time (source, &current_time);
  
  return ((timeout_source->expiration.tv_sec < current_time.tv_sec) ||
	  ((timeout_source->expiration.tv_sec == current_time.tv_sec) &&
	   (timeout_source->expiration.tv_usec <= current_time.tv_usec)));
}

static gboolean
g_timeout_dispatch (GSource    *source,
		    GSourceFunc callback,
		    gpointer    user_data)
{
#if 0	// as GLib
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  if (!callback)
    {
      g_warning ("Timeout source dispatched without callback\n"
		 "You must call g_source_set_callback().");
      return FALSE;
    }
 
  if (callback (user_data))
    {
      GTimeVal current_time;

      g_source_get_current_time (source, &current_time);
      g_timeout_set_expiration (timeout_source, &current_time);

      return TRUE;
    }
  else
    return FALSE;

#else
	GTimeoutSource *timeout_source = (GTimeoutSource *)source;
	GTimeVal current_time;

	g_source_get_current_time (source, &current_time);
    g_timeout_set_expiration (timeout_source, &current_time);

	return (callback (user_data)); 
#endif
}

GSourceFuncs g_timeout2_funcs =
{
  g_timeout_prepare,
  g_timeout_check,
  g_timeout_dispatch,
  NULL
};

GSource *
g_timeout2_source_new (guint interval)
{
  GSource *source = g_source_new (&g_timeout2_funcs, sizeof (GTimeoutSource));
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;
  GTimeVal current_time;

  timeout_source->interval = interval;

  g_get_current_time (&current_time);
  g_timeout_set_expiration (timeout_source, &current_time);
  
  return source;
}

guint
g_timeout2_add_full (gint           priority,
		    guint          interval,
		    GSourceFunc    function,
		    gpointer       data,
		    GDestroyNotify notify)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (function != NULL, 0);

  source = g_timeout2_source_new (interval);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);

  g_source_set_callback (source, function, data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

guint 
g_timeout2_add (guint32        interval,
	       GSourceFunc    function,
	       gpointer       data)
{
  return g_timeout2_add_full (G_PRIORITY_DEFAULT, 
			     interval, function, data, NULL);
}

// ---

void
g_timeout2_set_interval(guint source_id, guint32 interval)
{
	GSource *source = g_main_context_find_source_by_id(g_main_context_default(), source_id);
	GTimeoutSource *timeout_source = (GTimeoutSource *)source;

	timeout_source->interval = interval;	
}
