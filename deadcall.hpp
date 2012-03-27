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

  virtual bool process_incoming(char * msg, struct sockaddr_storage *);
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

