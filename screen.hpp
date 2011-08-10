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
 *  Copyright (C) 2003 - The Authors
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

/****
 * Screen.hpp : Simple curses & logfile encapsulation 
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
  void ERROR(const char *fmt, ...) __attribute__ ((noreturn));
  void WARNING(const char *fmt, ...);
  void ERROR_NO(const char *fmt, ...);
  void WARNING_NO(const char *fmt, ...);
  void MESSAGE(const char *fmt, ...);
  int _DEBUG_LOG(const char *fmt, ...);
  int _TRACE_MSG(const char *fmt, ...);
  int _TRACE_CALLDEBUG(const char *fmt, ...);
  int _TRACE_SHORTMSG(const char *fmt, ...);
  int _LOG_MSG(const char *fmt, ...);
  int _TRACE_EXEC(const char *fmt, ...);
#ifdef __cplusplus
}
#endif

#define EXIT_TEST_OK               0
#define EXIT_TEST_FAILED           1
#define EXIT_TEST_MANUALLY_STOPPED 95
#define EXIT_TEST_KILLED           96
#define EXIT_TEST_RES_INTERNAL     97
#define EXIT_TEST_RES_UNKNOWN      98
#define EXIT_OTHER                 99
#define EXIT_FATAL_ERROR           -1
#define EXIT_BIND_ERROR            -2
#define EXIT_SYSTEM_ERROR          -3

#define MAX_ERROR_SIZE            1024

#define DEBUG(x, ...) _DEBUG_LOG("%s() in %s:%d - " x "\n",  __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG_IN(x, ...) DEBUG("(Entered) - " x, ##__VA_ARGS__)
#define DEBUG_OUT(x, ...) DEBUG("(Leaving) - " x, ##__VA_ARGS__)

#define TRACE_MSG(x, ...)       { _TRACE_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define TRACE_CALLDEBUG(x, ...) { _TRACE_CALLDEBUG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define TRACE_SHORTMSG(x, ...)  { _TRACE_SHORTMSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define LOG_MSG(x, ...)         { _LOG_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }
#define TRACE_EXEC(x, ...)  { _TRACE_EXEC(x, ##__VA_ARGS__); _TRACE_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }

void screen_set_exename(char * exe_name);
void screen_init(void (*exit_handler)());
void screen_clear();
int  screen_readkey();
void screen_exit(int rc);
void screen_sigusr1(int /* not used */);

#endif // __SCREEN_H__

