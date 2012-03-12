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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

#ifndef __SIPP__
#define __SIPP__

/* Std C includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <time.h>
  #include <windows.h>

  #include "win32_compatibility.hpp"
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <sys/time.h>
  #include <sys/poll.h>
  #include <sys/resource.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <netdb.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <math.h>
#ifndef __SUNOS
#ifndef WIN32
  #include <curses.h>
#endif
#else
#include <stdarg.h>
#endif

#if defined(__HPUX) || defined(__SUNOS)
#include <alloca.h>
#endif

/* Sipp includes */
#include "sipp_globals.hpp"

#include "xp_parser.hpp"
#include "scenario.hpp"
#include "screen.hpp"
#include "task.hpp"
#include "listener.hpp"
#include "socketowner.hpp"
#include "call.hpp"
#include "comp.hpp"
#include "variables.hpp"
#include "stat.hpp"
#include "actions.hpp"
#include "infile.hpp"
#include "opentask.hpp"
#include "reporttask.hpp"
#include "watchdog.hpp"



/* Open SSL stuff */
#ifdef _USE_OPENSSL
#include "sslcommon.hpp" 
#endif

#ifdef WIN32
  #pragma warning (disable: 4003; disable: 4996)
#else
  #define SocketError() errno
#endif

#ifndef __CYGWIN
#ifndef FD_SETSIZE
#define FD_SETSIZE 65000
#endif
#else
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#endif


/************************** Constants **************************/

#ifdef SVN_VERSION
# ifdef LOCAL_VERSION_EXTRA
#  define SIPP_VERSION               SVN_VERSION LOCAL_VERSION_EXTRA
# else
#  define SIPP_VERSION               SVN_VERSION
# endif
#else
# define SIPP_VERSION               "unknown"
#endif


#ifdef GLOBALS_FULL_DEFINITION
#undef extern
#endif

#endif // __SIPP__
