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
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           Olivier Jacques
 *           From Hewlett Packard Company.
 *           Shriram Natarajan
 *           Peter Higginson
 *           Eric Miller
 *           Venkatesh
 *           Enrico Hartung
 *           Nasir Khan
 *           Lee Ballard
 *           Guillaume Teissier from FTR&D
 *           Wolfgang Beck
 *           Venkatesh
 *           Vlad Troyanker
 *           Charles P Wright from IBM Research
 *           Amit On from Followap
 *           Jan Andres from Freenet
 *           Ben Evans from Open Cloud
 *           Marc Van Diest from Belgacom
 *           Michael Dwyer from Cibation
 *           Polycom Inc. (Edward Estabrook, Richard Lum).  Contributions (c) 2010 - 2013
 */

#ifndef __DEADCALLHPP__
#define __DEADCALLHPP__

#include "listener.hpp"
#include "task.hpp"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

class deadcall : public virtual task, public virtual listener {
public:
  deadcall(char *id, const char * reason);
  ~deadcall();

  virtual bool process_incoming(char * msg, struct sockaddr_storage *, struct sipp_socket *socket);
  virtual bool  process_twinSippCom(char * msg);

  virtual bool run();

  /* When should this call wake up? */
  virtual unsigned int wake();

  /* Dump call info to error log. */
  virtual void dump();

protected:
  unsigned long expiration;
  char *reason;
};

#endif

