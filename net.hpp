/*
	Author:		R Brent Saunders
	Date:		Dec. 4, 2017

	Project:	Hypertac DFS NetDFP Driver

*/

//	HEADERS
//	-------

#ifndef UDP_SERVER_HPP
#define UDP_SERVER_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <data/dlinklist.hpp>
#include <ipc/genericio.h>
#include <datatypes.h>

#define MAX_DRIVER_MSG MAX_UDP_PACKET_SIZE
#define MAX_UDP_PACKET_SIZE	2048	// maximun bytes of any one message. adjustable by app

class UDPServer : public GenericIO {

	protected:
		int sock;				// handle to the socket
		int blocking;			// are we using blocking I/O ?
		struct sockaddr from;	// clients return address
		unsigned int fromlen;	// len of client address
		struct sockaddr qfrom;	// return address from fifo
		unsigned int qfromlen;	// len of address from fifo
		DoublyLinkedList clientList;
		DoublyLinkedList messageList;

		void init(short int);

	public:
		UDPServer();
		~UDPServer();

		void bindPort(const char *);// bind by service
		void bindPort(int);			// bind by port #
		void setNonBlocking();		// set to a non blocking socket.
		static int sameaddr(struct sockaddr *s1, int l1, struct sockaddr *s2, int l2);

		int readMessage(BYTE *, int);
		int sendMessage(BYTE *, int);
		int sendMessage(const char *buf) { return sendMessage((BYTE *)buf, strlen(buf)); }
		int checkMessages();
		int send(char *msg, struct sockaddr *cl, int len);
		void broadcast(char *sendbuf);
		void registerClient(void);	// add a client to the list of listeners
		void delClient(void);		// delete a client from list
		const char *getMessage();
		const char *popMessage();
};

class CLIENT : public Linkable {
	public:
	struct sockaddr *addr;						// handle to client requesting radio traffic
	int len;							// size of sockaddr, kept here to reduce computations
	long time;							// time since started, it will timeout after X seconds
};

class MESSAGE : public Linkable {

	public:
	struct sockaddr msgFrom;				// message return address
	unsigned int msgFromLen;
	int	msgType;
	char	msgData[MAX_DRIVER_MSG];
};
#endif

