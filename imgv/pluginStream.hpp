#ifndef IMGV_PLUGIN_STREAM_HPP
#define IMGV_PLUGIN_STREAM_HPP


/*!

\defgroup pluginStream Stream plugin
@{
\ingroup plugins

This plugin streams images over the net. It can both send and receive, but not at the same time.... (use two pluginStream if you need to do that)

Ports
-----

|      # | Name | Direction | Description |
| ------ | ---- | --------- | ----------- |
| 0      | in   | in        | input queue of images to send (or recycled when receiving) |
| 1      | out  | out       | output queue of received/sent images |
| 2      | cmd  | in        | input queue of messages to send to the network |
| 3      | log  | out       | input queue of images to send (or recycled when receiving) |

Events
------

none


OSC initialization-only commands
------------


### `/out <string host> <int port> <int mtu> <int delay>`

Set the stream to output all images to a specific host/port combination.<br>
The mtu is the maximum message size when sending images.<br>
Eventually, we should add multicast=on, compression level, and image size reduction.<br>
The delay is an optional time (in microsecond) between each packet sent. Keep to 0 normally.

### `/in <int port>`

Set the stream to listen to a port and recover images from there.<br>

### `/outtcp <string host> <int port>`

Setup outgoing tcp connexion.

### `/intcp <int port>`

Setup incoming tcp connexion.

### `/init-done`

Instruct that the initialization is finished, and the plugin can start
sending/receiving images.

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
#include <oscpack/ip/PacketListener.h>
#include <oscpack/ip/UdpSocket.h>


#include <imgv/tcpcast.h>



/// Stream images

///
/// /in <host> <port> -> set input mode, listen to specified port.
/// /out <host> <port> -> set output mode, send images to specified port.
///

class pluginStream : public plugin<blob>, PacketListener {
    public:
	static const int MODE_NONE = 0;
	static const int MODE_IN = 1;
	static const int MODE_OUT = 2;
	static const int MODE_IN_TCP = 3;
	static const int MODE_OUT_TCP = 4;
	static const int MODE_OUT_MSG_TCP = 5;
	static const int MODE_OUT_MSG = 6;
    private:
	bool initializing; // true: listen to .msg, false: listen to .in
	int mode; // MODE_*
	int n; // compteur interne utilise si count=true

	UdpTransmitSocket *transmitSocket; // NULL si unused
	UdpListeningReceiveSocket *receiveSocket; // NULL si unused

	// for tcp modes
	tcpcast tcp;

	unsigned char buf[2048]; // devrait etre > mtu

	int32 n0; // current image id (to identify the correct packets)
	blob *i0; // current received image
	int32 dsz; // current data size

	double scale; // image reduction factor (when sending images)

	int32 mtu; // taille d'un paquet udp
	int32 delay; // en microsecond
	bool realtime; // true: skip images. false: send all images
	string view; // undefined = keep what we have/get
    public:
	pluginStream();
	~pluginStream();
	void init();
	void uninit();
	bool loop();
    private:
	bool decode(const osc::ReceivedMessage &m);
	// de PacketListener
	void ProcessPacket( const char *data, int size,
		 const IpEndpointName& remoteEndpoint );
	void ProcessPacket( const char *data, int size );
	int checksum(unsigned char *data,int size);


};


#endif

