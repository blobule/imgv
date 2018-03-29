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
 * along with BMC.  If not, see <http://www.gnu.org/licenses/>.
 */


/*** librairie udpcast simple ***/

#include "udpcast.h"

#include <errno.h>

// type : UDP_TYPE_NORMAL, _BROADCAST, _MULTICAST = _MULTICAST_LOOP, _MULTICAST_NOLOOP

int udp_init_sender(udpcast *uc,const char *dst_addr,int dst_port,int type)
{
//int status;

#ifdef WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,0);
  if (WSAStartup(wVersionRequested,&wsaData)!=0)
  {
    printf("tcp server: unable to allocate server socket");
    return(-1);
  }
#endif

   // set content of struct saddr to zero
   memset(&uc->saddr, 0, sizeof(struct sockaddr_in)); // source

   // open a UDP socket
   uc->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // ou ,0); 
   if ( uc->sock < 0 ) { printf("Error creating socket");return(-1); }


   struct hostent *hp;
   hp = gethostbyname(dst_addr);
   if( hp==0 ) { printf("bad host?\n");return(-1); }
   memcpy((char *)&uc->saddr.sin_addr, (char *)hp->h_addr, hp->h_length);

   switch( type ) {
    case UDP_TYPE_BROADCAST:
	//uc->saddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	{
	int broadcast=1;
	if( setsockopt(uc->sock, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof(broadcast)) == -1 ) {
		printf("unable to get broadcast\n");
		return(-1);
	}
	break;
	}
    case UDP_TYPE_MULTICAST:
    case UDP_TYPE_MULTICAST_LOOP:
	{
	struct in_addr iaddr;
	iaddr.s_addr = htonl(INADDR_ANY); // use any interface
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr, sizeof(struct in_addr));

	// Set multicast packet TTL to 3; default TTL is 1
	unsigned char ttl=1;
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,sizeof(ttl));

	// send multicast traffic to myself too (one=yes or zero=no)
	unsigned char loop=1;
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	break;
	}
    case UDP_TYPE_MULTICAST_NOLOOP:
	{
	struct in_addr iaddr;
	iaddr.s_addr = htonl(INADDR_ANY); // use any interface
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr, sizeof(struct in_addr));

	// Set multicast packet TTL to 3; default TTL is 1
	unsigned char ttl=1;
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,sizeof(ttl));

	// send multicast traffic to myself too (one=yes or zero=no)
	unsigned char loop=0;
	setsockopt(uc->sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	break;
	}
    case UDP_TYPE_NORMAL:
	break;
   }


   uc->saddr.sin_family = AF_INET;
   uc->saddr.sin_port = htons(dst_port);

   //status = bind(uc->sock, (struct sockaddr *)&uc->saddr, sizeof(struct sockaddr_in));
   //if( status<0 ) { printf("Unable to bind to an outgoing socket. failed. status=%d.\n",status);return(-1); }

   //uc->saddr.sin_family = AF_INET;
   //uc->saddr.sin_port = htons(dst_port);
   //uc->saddr.sin_addr.s_addr = inet_addr(dst_addr);

	// on peut faire connect, puis utiliser send
	// sinon, sans connect, on utilise sendto
/***
   if( connect(uc->sock, (struct sockaddr *) &uc->saddr, sizeof(struct sockaddr_in)) < 0 ) {
		printf("unable to connect\n");
		return(-1);
	}
***/

   return(0);
}


int udp_send_data(udpcast *uc,unsigned char *buffer,int bufferSize)
{
int status;

   // send data
   status = sendto(uc->sock, buffer, bufferSize, 0,
	     (struct sockaddr *)&uc->saddr, sizeof(struct sockaddr_in));

	// si on a utilise connect
   //status = send(uc->sock, buffer, bufferSize, 0);

   return(status);
}

int udp_uninit_sender(udpcast *uc)
{
#ifdef WIN32
	shutdown(uc->sock, SD_BOTH);
    closesocket(uc->sock);
	WSACleanup();
#else
    shutdown(uc->sock, SHUT_RDWR);
    close(uc->sock);
#endif
   return(0);
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////


// retourne 0 si ok, -1 si unable to bind
// NOTE: unable to bind means that we can not receive replies...
//       For a controller, this means we can't use send_safe but send_unsafe is ok
//       For a receiver, this is fatal.
//       mcast_group is 226.0.0.1 (ou autre) si on est en multicast, sinon NULL
int udp_init_receiver(udpcast *uc,int in_port,char *mcast_group)
{
int status;

#ifdef WIN32
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,0);
  if (WSAStartup(wVersionRequested,&wsaData)!=0)
  {
    printf("tcp server: unable to allocate server socket");
    return(-1);
  }
#endif

   // set content of struct saddr to zero
   memset(&uc->saddr, 0, sizeof(struct sockaddr_in));

   // open a UDP socket
   uc->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( uc->sock < 0 ) { printf("Error creating socket");return(-1); }

   // allow multiple dudes to bind to this socket
   // de toute facon, un seul bond recoit les messages...
/*****
   u_int yes=1;
   status = setsockopt(uc->sock, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(yes));
   if( status<0 ) { printf("resuse is not possible\n");return(-1); }
*****/

   uc->saddr.sin_family = AF_INET;
   uc->saddr.sin_port = htons(in_port);
   uc->saddr.sin_addr.s_addr = htonl(INADDR_ANY); // bind socket to any interface
   status = bind(uc->sock, (struct sockaddr *)&uc->saddr, sizeof(struct sockaddr_in));

   if ( status < 0 ) {
	   printf("Error bind socket for listening\n");
	   close(uc->sock);
	   uc->sock=-1;
	   return(-999); // special code since this error is not fatal for unsafe controllers
   }

   if( mcast_group ) {
	struct ip_mreq imreq;
	imreq.imr_multiaddr.s_addr = inet_addr("226.0.0.1");
	imreq.imr_interface.s_addr = INADDR_ANY; // use DEFAULT interface

	// JOIN multicast group on default interface
	status = setsockopt(uc->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(const void *)&imreq, sizeof(struct ip_mreq));
	printf("MC status is %d\n",status);
   }

   return(0);
}



int udp_receive_data(udpcast *uc,unsigned char *buffer,int bufferSize)
{
int status;
int socklen;

   if( uc->sock<0 ) return(0); // nothing!

   // receive packet from socket
   socklen = sizeof(struct sockaddr_in);
   status = recvfrom(uc->sock, buffer, bufferSize, 0, 
		     (struct sockaddr*)&uc->saddr, (socklen_t*)&socklen);
   if( status>=0 ) buffer[status]=0;
   else printf("[[[udpcast]]] errno=%d (%s)\n",errno,strerror(errno));
   //printf("Got '%s' len=%d\n",buffer,status);
   return(status);
}


int udp_receive_data_poll(udpcast *uc,unsigned char *buffer,int bufferSize)
{
fd_set readset;
struct timeval timeout;

  if( uc->sock<0 ) return(0); // nothing!

  timeout.tv_sec = 0;
  timeout.tv_usec = 1;

  FD_ZERO(&readset);
  FD_SET(uc->sock,&readset);

  select(uc->sock+1, &readset, NULL,NULL,&timeout);
  if( FD_ISSET(uc->sock,&readset) == 0 ) return(0); // nothing!

  return(udp_receive_data(uc,buffer,bufferSize));
}

int udp_uninit_receiver(udpcast *uc)
{
   if( uc->sock>=0 ) {
#ifdef WIN32
  	  shutdown(uc->sock, SD_BOTH);
      closesocket(uc->sock);
	  WSACleanup();
#else
      shutdown(uc->sock, SHUT_RDWR);
      close(uc->sock);
#endif
   }
   return(0);
}


