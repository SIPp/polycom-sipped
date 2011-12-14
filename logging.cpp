/*
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author : Edward Estabrook - 13 Dec 2011
 *           From Polycom
 *           Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

/****
 * Logging.cpp : Simple logfile encapsulation 
 */


#include "win32_compatibility.hpp"
#include "logging.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#ifndef WIN32
# include <sys/time.h>
# include <unistd.h>
# include <signal.h>
#endif


// Variables set by configuration parameters 
unsigned long long max_log_size = 0;
unsigned long long ringbuffer_size = 0;
int ringbuffer_files = 0;

// Log file name set by configuration parameters
#define LOGFILE(name, s, check) \
	struct logfile_info name = { s, check, NULL, 0, NULL, "", true, false, 0, 0};
LOGFILE(calldebug_lfi, "calldebug", true);
LOGFILE(message_lfi, "messages", true);
LOGFILE(shortmessage_lfi, "shortmessages", true);
LOGFILE(log_lfi, "logs", true);
LOGFILE(error_lfi, "errors", false);
LOGFILE(debug_lfi, "debug", false);
LOGFILE(exec_lfi, "exec", false);

// Needed by screen to tell user what file to look in for more errors.
char screen_logfile[MAX_PATH] = "";

char scenario_file_name[MAX_PATH] = "scenario_file_name_not_set_in_logging";

void set_logging_scenario_file_name(char *name)
{
  if (!name) {
    fprintf(stderr, "ERROR: Null scenario name passed into set_scenario_file_name.\n");
    assert(0);
  }

  strncpy(scenario_file_name, name, MAX_PATH-1);
}

// return true if log file opened, false if not.
int rotatef(struct logfile_info *lfi) {
  char L_rotate_file_name [MAX_PATH];

  if (!lfi->fixedname) {
    sprintf (lfi->file_name, "%s_%d_%s.log", scenario_file_name, getpid(), lfi->name);
  }

  if (ringbuffer_files > 0) {
    if (!lfi->ftimes) {
      lfi->ftimes = (struct logfile_id *)calloc(ringbuffer_files, sizeof(struct logfile_id));
    }
    // We need to rotate away an existing file. 
    if (lfi->nfiles == ringbuffer_files) {
      if ((lfi->ftimes)[0].n) {
        sprintf(L_rotate_file_name, "%s_%d_%s_%lu.%d.log", scenario_file_name, getpid(), lfi->name, (lfi->ftimes)[0].start, (lfi->ftimes)[0].n);
      } else {
        sprintf(L_rotate_file_name, "%s_%d_%s_%lu.log", scenario_file_name, getpid(), lfi->name, (lfi->ftimes)[0].start);
      }
      unlink(L_rotate_file_name);
      lfi->nfiles--;
      memmove(lfi->ftimes, &((lfi->ftimes)[1]), sizeof(struct logfile_id) * (lfi->nfiles));
    }
    if (lfi->starttime) {
      (lfi->ftimes)[lfi->nfiles].start = lfi->starttime;
      (lfi->ftimes)[lfi->nfiles].n = 0;
      // If we have the same time, then we need to append an identifier. 
      if (lfi->nfiles && ((lfi->ftimes)[lfi->nfiles].start == (lfi->ftimes)[lfi->nfiles - 1].start)) {
        (lfi->ftimes)[lfi->nfiles].n = (lfi->ftimes)[lfi->nfiles - 1].n + 1;
      }
      if ((lfi->ftimes)[lfi->nfiles].n) {
        sprintf(L_rotate_file_name, "%s_%d_%s_%lu.%d.log", scenario_file_name, getpid(), lfi->name, (lfi->ftimes)[lfi->nfiles].start, (lfi->ftimes)[lfi->nfiles].n);
      } else {
        sprintf(L_rotate_file_name, "%s_%d_%s_%lu.log", scenario_file_name, getpid(), lfi->name, (lfi->ftimes)[lfi->nfiles].start);
      }
      lfi->nfiles++;
      fflush(lfi->fptr);
      fclose(lfi->fptr);
      lfi->fptr = NULL;
      rename(lfi->file_name, L_rotate_file_name);
    }
  }

  time(&lfi->starttime);
  if (lfi->overwrite) {
    lfi->fptr = fopen(lfi->file_name, "w");
  } else {
    lfi->fptr = fopen(lfi->file_name, "a");
    if (lfi->fptr) 
      lfi->overwrite = true; // only set 'overwrite' if open was successful.
  }
  if(lfi->check && !lfi->fptr) {
    // We can not use the error functions from this function, as we may be rotating the error log itself! 
    printf("\nERROR: Unable to open/create '%s'\n", lfi->file_name);
    assert(0);
  }
  return (lfi->fptr != 0);
}

void rotate_calldebugf() {
  rotatef(&calldebug_lfi);
}

void rotate_messagef() {
  rotatef(&message_lfi);
}


void rotate_shortmessagef() {
  rotatef(&shortmessage_lfi);
}


void rotate_logfile() {
  rotatef(&log_lfi);
}

void rotate_errorf() {
  rotatef(&error_lfi);
  strcpy(screen_logfile, error_lfi.file_name);
}

void rotate_debugf() {
  rotatef(&debug_lfi);
  setvbuf(debug_lfi.fptr, (char *)NULL, _IONBF, 0);
}

void rotate_execf() {
  rotatef(&exec_lfi);
  setvbuf(exec_lfi.fptr, (char *)NULL, _IONBF, 0);
}



int _trace (struct logfile_info *lfi, const char *fmt, va_list ap) {
  int ret = 0;
  if(lfi->fptr) {
    ret = vfprintf(lfi->fptr, fmt, ap);
    fflush(lfi->fptr);

    lfi->count += ret;

    if (max_log_size && lfi->count > max_log_size) {
      fclose(lfi->fptr);
      lfi->fptr = NULL;
    }

    if (ringbuffer_size && lfi->count > ringbuffer_size) {
      rotatef(lfi);
      lfi->count = 0;
    }
  }
  return ret;
}

int _DEBUG_LOG(const char *fmt, ...) {
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = _trace(&debug_lfi, fmt, ap);
  va_end(ap);

  return ret;
}

int _TRACE_MSG(const char *fmt, ...) {
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = _trace(&message_lfi, fmt, ap);
  va_end(ap);

  return ret;
}

int _TRACE_SHORTMSG(const char *fmt, ...) {
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = _trace(&shortmessage_lfi, fmt, ap);
  va_end(ap);

  return ret;
}

int _LOG_MSG(const char *fmt, ...) {
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = _trace(&log_lfi, fmt, ap);
  va_end(ap);

  return ret;
}

int _TRACE_CALLDEBUG(const char *fmt, ...) {
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = _trace(&calldebug_lfi, fmt, ap);
  va_end(ap);

  return ret;
}


