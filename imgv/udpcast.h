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

#ifndef UDP_H
#define UDP_H

/*** librairie multicast simple ***/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef WIN32
  #include <winsock2.h>
  
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netdb.h>
#endif
//#include <netinet/in.h>

// parce que linux/in.h pas compatible avec netinet/in.h
#define IP_MTU 14 /* From <linux/in.h> */ 

#include <string.h>
//#include <sys/select.h>
//#include <arpa/inet.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
   // sender & receiver
   int sock;
   struct sockaddr_in saddr;
} udpcast;

#define UDP_TYPE_NORMAL	0
#define UDP_TYPE_BROADCAST	1
#define UDP_TYPE_MULTICAST	2
#define UDP_TYPE_MULTICAST_LOOP	3
#define UDP_TYPE_MULTICAST_NOLOOP	4

int udp_init_sender(udpcast *uc,const char *dst_addr,int dst_port,int type);

int udp_send_data(udpcast *uc,unsigned char *buffer,int bufferSize);

int udp_uninit_sender(udpcast *uc);


//       mcast_group is 226.0.0.1 (ou autre) si on est en multicast, sinon NULL
int udp_init_receiver(udpcast *uc,int in_port,char *mcast_group);

int udp_receive_data(udpcast *uc,unsigned char *buffer,int bufferSize);
// retourn 0 si pas de data a lire
int udp_receive_data_poll(udpcast *uc,unsigned char *buffer,int bufferSize);
int udp_uninit_receiver(udpcast *uc);

#ifdef __cplusplus
}
#endif

#endif
