//	HEADERS
//	-------

#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <ipc/genericio.h>
#include <datatypes.h>

#define MAX_TCP_CONNECTIONS	200			// safe max for most apps
#define MAX_TCP_MESSAGE 4096

class TcpServer : public GenericIO {

	protected:
		int sock;
		fd_set socks;
		int highsock;
		int lastsock;
		struct sockaddr_in server_address;
		struct timeval timeout;
		int tv_sec, tv_usec;
		int readsocks;
		int socknum;
		int num_sockets;
		int *connectlist;
		int *lastActivityList;

		void buildSelectList();				// who's got data incoming
		void handleNewConnection();			// new connection request
		void cleanConnectionList();			// remove stale connections
		void setNonBlocking(int);			// set to a non blocking socket.
		int sendMessage(int, const char *);	// socket and null terminated string
		int sendMessage(int, const BYTE *, int);	// socket, buffer, len
		FLAG init(short int);

	public:
		TcpServer();					// default to 1 connection
		TcpServer(int);					// max number of connections for this server
		virtual ~TcpServer();

		// setup the timeout for the select call
		void setTimeout(int sec, int usec);

		// initialize and start listening
		FLAG bindPort(const char *);		// by tcp service in /etc/services
		FLAG bindPort(int);					// by port number

		FLAG isMessageReady(void);			// 0 for no, socket id if yes
		int readMessage(BYTE *, int);		// read len bytes from socket if message was ready

		int sendMessage(BYTE *, int);		// send to socket that was last received from using readMessage()
		int sendMessage(BYTE *, int, int);	// buffer, len, clientsocket index

		void flushSocket(int);				// flush the socket
		void closeSocket(int);				// close the socket index in connectlist
		FLAG prepareBroadcast();			// return true if there are any sockets to broadcast to
											// call this before broadcasting.  Only need to call once between
											// calls to isMessageReady()
		FLAG broadcast(const BYTE *, int);
		FLAG broadcast(const char *buf) { return broadcast((BYTE *)buf, strlen(buf)); }

		void clearAllConnections();			// remove all connections
		void cleanMyConnections();

		// diagnostic routines
		void listAllConnections();
		int numConnections();				// return number of connected sockets
		int getLastSocketIndex() { return lastsock; }
};
#endif

