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
 *  Copyright (C) 2011 - The Authors
 *
 *  Author : Edward Estabrook - 13 Dec 2011
 *           From Polycom
 *           Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

/****
 * logging.hpp : Simple logfile encapsulation
 */

#ifndef __LOGGING__
#define __LOGGING__

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "win32_compatibility.hpp"


void set_logging_scenario_file_name(char *name);

void rotate_messagef();
void rotate_calldebugf();
void rotate_shortmessagef();
void rotate_logfile();
void rotate_debugf();
void rotate_execf();

// Variables set by configuration parameters
extern unsigned long long max_log_size;
extern unsigned long long ringbuffer_size;
extern int ringbuffer_files;

// Needed by screen to tell user what file to look in for more errors.
extern char screen_logfile[MAX_PATH];


struct logfile_id {
  time_t start;
  int n;
};

struct logfile_info {
  const char *name;
  bool check;
  FILE *fptr;
  int nfiles;
  struct logfile_id *ftimes;
  char file_name[MAX_PATH];
  bool overwrite;
  bool fixedname;
  time_t starttime;
  unsigned int count;
};


extern struct logfile_info calldebug_lfi;
extern struct logfile_info message_lfi;
extern struct logfile_info shortmessage_lfi;
extern struct logfile_info log_lfi;
extern struct logfile_info error_lfi;
extern struct logfile_info debug_lfi;
extern struct logfile_info exec_lfi;

int _trace (struct logfile_info *lfi, const char *fmt, va_list ap);

int _DEBUG_LOG(const char *fmt, ...);
int _TRACE_MSG(const char *fmt, ...);
int _TRACE_SHORTMSG(const char *fmt, ...);
int _LOG_MSG(const char *fmt, ...);
int _TRACE_CALLDEBUG(const char *fmt, ...);

#define DEBUG(x, ...) _DEBUG_LOG("%s() in %s:%d - " x "\n",  __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG_IN(x, ...) DEBUG("(Entered) - " x, ##__VA_ARGS__)
#define DEBUG_OUT(x, ...) DEBUG("(Leaving) - " x, ##__VA_ARGS__)

#define TRACE_MSG(x, ...)       { _TRACE_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define TRACE_CALLDEBUG(x, ...) { _TRACE_CALLDEBUG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define TRACE_SHORTMSG(x, ...)  { _TRACE_SHORTMSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define LOG_MSG(x, ...)         { _LOG_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }

#endif // __LOGGING__
