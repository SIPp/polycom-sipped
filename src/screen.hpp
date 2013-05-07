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
 *           Polycom Inc. (Edward Estabrook, Richard Lum, Daniel Busto).  Contributions (c) 2010 - 2013
 */

/****
 * Screen.hpp : Simple curses & logfile encapsulation
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

#ifndef WIN32
void REPORT_ERROR(const char *fmt, ...) __attribute__ ((noreturn));
#else
void REPORT_ERROR(const char *fmt, ...);
#endif
void WARNING(const char *fmt, ...);
void REPORT_ERROR_NO(const char *fmt, ...);
void WARNING_NO(const char *fmt, ...);
void MESSAGE(const char *fmt, ...);

// _TRACE_EXEC defined here to as it has dependencies that don't belong in logging.
int _TRACE_EXEC(const char *fmt, ...);

#define TRACE_EXEC(x, ...)  { _TRACE_EXEC(x, ##__VA_ARGS__); _TRACE_MSG(x, ##__VA_ARGS__); _DEBUG_LOG(x, ##__VA_ARGS__); }

void screen_set_exename(char * exe_name);
void screen_init(void (*exit_handler)(), void (*releaseGlobalAllocations_handler)());
void screen_clear();
int  screen_readkey();
void screen_exit(int rc);

#define EXIT_TEST_OK               0
#define EXIT_TEST_FAILED           1
#define EXIT_SCREEN_UNITTEST       94
#define EXIT_TEST_MANUALLY_STOPPED 95
#define EXIT_TEST_KILLED           96
#define EXIT_TEST_RES_INTERNAL     97
#define EXIT_TEST_RES_UNKNOWN      98
#define EXIT_OTHER                 99
#define EXIT_FATAL_ERROR           255
#define EXIT_BIND_ERROR            254
#define EXIT_SYSTEM_ERROR          253
#define EXIT_ARGUMENT_ERROR        252

#define MAX_ERROR_SIZE            1024

#ifdef WIN32
const int KEY_BACKSPACE_SIPP = 8;
const int KEY_DC_SIPP = 127;
#elif __SUNOS
const int KEY_BACKSPACE_SIPP = 14;
const int KEY_DC_SIPP = 14;
#else
const int KEY_BACKSPACE_SIPP = 0x7e;
const int KEY_DC_SIPP = 0x7f;
#endif

#endif // __SCREEN_H__

