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

#ifndef TCPCAST_H
#define TCPCAST_H

/*** librairie tcpcast simple ***/



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
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
   int server_sock; //used internally, for server only
   int remote_sock; // -1 it not connected.
} tcpcast;


int tcp_server_init(tcpcast *tc,int port);
int tcp_client_init(tcpcast *tc,const char *srv_addr,int srv_port);

void tcp_server_wait(tcpcast *tc);
int tcp_server_is_connected(tcpcast *tc); // 0=not, 1=connected
int tcp_server_still_connected(tcpcast *tc); // 0=no 1=yes

int tcp_send_data(tcpcast *tc,const char *buffer,int bufferSize);
int tcp_send_string(tcpcast *tc,const char *buffer);
int tcp_receive_data(tcpcast *tc,const char *buffer,int bufferSize);
int tcp_receive_data_exact(tcpcast *tc,const char *buffer,int bufferSize);

void tcp_server_close_connection(tcpcast *tc);
void tcp_server_close(tcpcast *tc);
void tcp_client_close(tcpcast *tc);


#ifdef __cplusplus
}
#endif

#endif
