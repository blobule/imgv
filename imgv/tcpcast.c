/*
 * This file is part of BMC.
 * 
 * @Copyright 2008 Université de Montréal, Laboratoire Vision3D
 *   Sébastien Roy (roys@iro.umontreal.ca)
 *   Louis Bouchard (lwi.bouchard@gmail.com)
 *
 * BMC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BMC.  If not, see <http://www.gnu.org/licenses/>.port
 */

/**
 * @file
 * Simple tcp library
 *
 * <h2>Usagee</h2>
 * 
 * This first usage if to measure the time spent in a piece of code.
 *
 * @code
 * #define USE_PROFILOMETRE
 * #include <profilometre.h>
 *
 * ...
 *
 * // Must call init before any other from the library
 * profilometre_init();
 *
 * ...
 *
 * profilometre_start("example");
 * do_something();
 * profilometre_stop("exemple");
 *
 * ...
 * 
 * profilometre_dump();
 * @endcode
*/



#include "tcpcast.h"

/**
 * initialize tcp server
 *
*/
int tcp_server_init(tcpcast *tc,int port)
{
int status;
struct sockaddr_in localAddr;

#ifdef WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,0);
  if (WSAStartup(wVersionRequested,&wsaData)!=0)
  {
    printf("tcp server: unable to allocate server socket");
    return(-1);
  }
#endif

   tc->remote_sock=-1; // not connected.

   // set content of struct saddr to zero
   memset(&localAddr,0,sizeof(localAddr));
   localAddr.sin_port = htons(port);
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

   tc->server_sock = socket(AF_INET, SOCK_STREAM, 0);
   if ( tc->server_sock < 0 ) { printf("tcp server: error creating socket.\n");return(-1); }
   status = bind(tc->server_sock,(struct sockaddr *)&localAddr,sizeof(localAddr));
   if( status<0 ) 
   {
 	 printf("tcp server: unable to bind to an outgoing socket. failed. status=%d.\n",status);
	 return(-1);
   }
   listen(tc->server_sock,5); //besoin d'une file d'attente??

   return(0);
}

/**
 * TCP client initialization.
 *
*/
int tcp_client_init(tcpcast *tc,const char *srv_addr,int srv_port)
{
struct sockaddr_in serverAddr;
struct hostent *serverHostEnt;
long hostAddr;

#ifdef WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,0);
  if (WSAStartup(wVersionRequested,&wsaData)!=0)
  {
    printf("tcp client: unable to allocate client socket");
    return(-1);
  }
#endif  

  memset(&serverAddr,0,sizeof(serverAddr));
  hostAddr = inet_addr(srv_addr);
  if ( (long)hostAddr != (long)-1)
  {
    memcpy(&serverAddr.sin_addr,&hostAddr,sizeof(hostAddr));
  }
  else
  {
    serverHostEnt = gethostbyname(srv_addr);
    if (serverHostEnt == NULL) { printf("get hostbyname!\n"); return -1;}
    memcpy(&serverAddr.sin_addr,serverHostEnt->h_addr,serverHostEnt->h_length);
  }
  serverAddr.sin_port = htons(srv_port);
  serverAddr.sin_family = AF_INET;
  
  //socket creation
  if ( (tc->remote_sock = socket(AF_INET,SOCK_STREAM,0)) < 0) { printf("no socket!\n");return -1;}
  //connexion request
  if(connect( tc->remote_sock,
            (struct sockaddr *)&serverAddr,
            sizeof(serverAddr)) < 0 )
  {
    printf("tcp client: unable to connect.\n");
    return -1;
  }

  return 0;
}

//#include <errno.h>

void tcp_server_wait(tcpcast *tc)
{
struct sockaddr_in clientAddr;
int address_len;

   address_len = sizeof(clientAddr);
   tc->remote_sock = accept(tc->server_sock,
                         (struct sockaddr *)&clientAddr,
                         (socklen_t *)(&address_len));
   if( tc->remote_sock<0 ) {
		printf("tcp accept error\n");
   }
}

int tcp_send_data(tcpcast *tc,const char *buffer,int bufferSize)
{
int status;

   // send data
#ifdef WIN32
   status = send(tc->remote_sock, buffer, bufferSize,0);
#else
   status = write(tc->remote_sock, buffer, bufferSize);
#endif
   return(status);
}

int tcp_send_string(tcpcast *tc,const char *buffer)
{
  return tcp_send_data(tc,buffer,strlen(buffer)+1);
}

// can return less bytes than expected
int tcp_receive_data(tcpcast *tc,const char *buffer,int bufferSize)
{
int status;
   // receive data
   if( !tcp_server_still_connected(tc) ) return -1;
#ifdef WIN32
   status = recv(tc->remote_sock, (void *)(buffer), bufferSize,0);
#else
   status = read(tc->remote_sock, (void *)(buffer), bufferSize);
#endif
   return(status);
}

// get an exact number of bytes. return 0 if lost connexion, otherwise buffersize
int tcp_receive_data_exact(tcpcast *tc,const char *buffer,int bufferSize)
{
	int i=0;
	while( i<bufferSize ) {
		int nb=tcp_receive_data(tc,buffer+i,bufferSize-i);
		if( nb<0 ) return(0);
		i+=nb;
	}
	return bufferSize;
}

void tcp_server_close_connection(tcpcast *tc)
{
   if( tc->remote_sock>=0 ) {
#ifdef WIN32
	   shutdown(tc->remote_sock,SD_BOTH);
	   closesocket(tc->remote_sock);
#else
	   shutdown(tc->remote_sock,SHUT_RDWR);
	   close(tc->remote_sock);
#endif
	   tc->remote_sock=-1; // not connected
   }
}

void tcp_server_close(tcpcast *tc)
{
   //tcp_client_close(tc);
   tcp_server_close_connection(tc);
#ifdef WIN32
   closesocket(tc->server_sock);
   WSACleanup();
#else
   close(tc->server_sock);
#endif
}

void tcp_client_close(tcpcast *tc)
{
   if( tc->remote_sock>=0 ) {
#ifdef WIN32
	   shutdown(tc->remote_sock,SD_BOTH);
	   closesocket(tc->remote_sock);
	   WSACleanup();
#else
	   shutdown(tc->remote_sock,SHUT_RDWR);
	   close(tc->remote_sock);
#endif
	   tc->remote_sock=-1; // not connected
   }
}

int tcp_server_is_connected(tcpcast *tc)
{
	return( tc->remote_sock>=0 );
}

int tcp_server_still_connected(tcpcast *tc)
{
	if( tc->remote_sock<0 ) return 0;
	// actual check
	char c;
	return( recv(tc->remote_sock,&c, 1, MSG_PEEK)!=0 );

}


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

