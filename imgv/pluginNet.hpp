#ifndef IMGV_PLUGIN_NET_HPP
#define IMGV_PLUGIN_NET_HPP


/*!

\defgroup pluginNet Net plugin
@{
\ingroup plugins

This plugin handles the forwarding of messages (images or control) over the network. It can send as well as receive, and support udp (unicast and multicast) as well as tcp.

Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | input queue of messages/images to send (or recycled images when receiving) |
| 1      | out  | out       | output queue of received messages/images (or send images)|
| 2      | cmd  | in        | input queue of messages to control this plugin |
| 3      | log  | out       | log info |

Events
------

| Name | Description |
| ---- | ----------- |
| image | An image was sent over the network. ($x contains image number) |


OSC initialization-only commands
------------

### `/out/udp <string host> <int32 port> <int32 delay>`

Set the net plugin to output all images to a specific host/port combination.<br>
The mtu is the maximum message size when sending images.<br>
The delay is in microseconds, and is waited after sending each udp packet.

### `/out/multicast <string host> <int32 port> <int32 delay>`

Set the net plugin to output all images to a multicast host at a specified port. A typical addresse is 226.0.0.1.<br>
The delay is in microseconds, and is waited after sending each udp packet.

### `/out/tcp <string host> <int port>`

Setup outgoing tcp connexion.

### `/in/udp <int port>`

Set the stream to listen to a port and recover images from there.<br>

### `/in/multicast <string ip> <int port>`

reception multicast

### `/in/tcp <int port>`

Setup incoming tcp connexion.

OSC anytime commands
------------

### /real-time <true|false>

Indicate if all images should be sent over the network (=false) or if only
as many images as the bandwidth allows should be sent (=true). <br>
In all cases, all the images are eventually sent to the output port.<br>
Default to true.<br>
(this command can also be sent during initialization)

### /scales \<float\>

Set the image scaling to use when sending images out.<br>
Default to 1.<br>
(this command can also be sent during initialization)

### /view <string>

Reset the view of received image, or the view sent out

OSC after-initialization commands
------------


Example 1
----------

Stream to 192.168.1.100 port 44444 ...

~~~
/real-time false
/out 192.168.1.100 44444 1450
/init-done
~~~

Example 2
----------

Receive images from network port 44444 ...

~~~
/in 44444
/init-done
~~~


Sample code
------------------
- \ref netCamera


Todo
----------

- add commands to stop sending or stop listening
- remove init-done command
- allow sending and listening at the same time

\todo update doc

@}
*/




#include <imgv/imgv.hpp>

#include <imgv/plugin.hpp>

#include <oscpack/osc/OscOutboundPacketStream.h>
//#include <oscpack/ip/PacketListener.h>
//#include <oscpack/ip/UdpSocket.h>


#include <imgv/tcpcast.h>
#include <imgv/udpcast.h>



/// Net send/receive

///
/// /in <host> <port> -> set input mode, listen to specified port.
/// /out <host> <port> -> set output mode, send images to specified port.
///

class pluginNet : public plugin<blob> {
    public:
	static const int MODE_NONE = 0;
	static const int MODE_IN = 1;
	static const int MODE_OUT = 2;

	static const int TYPE_TCP = 0;
	static const int TYPE_UDP = 1;
	static const int TYPE_MULTICAST = 2;
    private:
	int mode; // MODE_*. MODE_NONE: do nothing.
	int type; // TYPE_* Note that 
	int n; // compteur interne utilise si count=true

	//UdpTransmitSocket *transmitSocket; // NULL si unused
	//UdpListeningReceiveSocket *receiveSocket; // NULL si unused

	// for tcp modes
	tcpcast tcp;

	// for udp modes
	udpcast udp;

	unsigned char *buf; // devrait etre > mtu

	//
	// sending images
	//
	int mtu; // taille d'un paquet udp
	double scale; // image reduction factor (when sending images)
	int delay; // en microsecond, when sending images
	string view; // undefined = keep what we have/get

	//
	// receiving images
	//
	int32 n0; // current image id (to identify the correct packets)
	blob *i0; // current received image
	int32 dsz; // current data size
	bool realtime; // true: skip images. false: send all images

	bool debug; // use /debug T/F to set this

    public:
	pluginNet();
	~pluginNet();
	void init();
	void uninit();
	bool loop();
    private:
	bool decode(const osc::ReceivedMessage &m);
	// de PacketListener
	//void ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint );
	//void ProcessPacket( const char *data, int size );
	int checksum(unsigned char *data,int size);
	void sendImage(blob *i1);
	void receiveImageStart(const osc::ReceivedMessage &m);
	void receiveImageData(const osc::ReceivedMessage &m);
	void processInMessage(unsigned char *buf,int sz);

};


#endif

