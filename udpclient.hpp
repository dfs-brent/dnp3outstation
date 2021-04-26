#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <ipc/genericio.h>
#include <datatypes.h>

#ifndef UDPCLIENT_HPP
#define UDPCLIENT_HPP

class UDPClient : GenericIO {

  protected:

    int sock;
    struct sockaddr_in cpy;

  public:
    UDPClient();
    virtual ~UDPClient();

    int getSocket();
    void bindService(const char *);
    void setupBroadcast(const char *);          // broadcast on a port
    void setSockaddr(struct sockaddr_in *);
    static void makeRecipient(struct sockaddr_in *server, const char *, const char *);
    int setNonBlocking(int);

    int readMessage(BYTE *, int);
    int sendMessage(BYTE *, int);
    int sendMessage(BYTE *, int, struct sockaddr_in *);
  int sendMessage(const char *buf) { return sendMessage((BYTE *)buf, strlen(buf)); }
    int sendMessage(const char *buf,struct sockaddr_in *srv) { return(sendMessage((BYTE *)buf,strlen(buf),srv)); }
};

#endif
