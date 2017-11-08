# Minetest network protocol (v2)

## Presentation
 
Minetest protocol v2 multiplex TCP and UDP communication channels

* TCP is used for reliable communications
* UDP is used for unreliable communications

## Connection protocol

Minetest connects to server by opening a TCP socket to the exposed TCP port.

After succeed connection and player is on the map, client knows his session ID. This is
the only chance to get it after a successful connection.

UDP communication channel is opened by client, sending regular pings (~5sec) to server.
Those pings are received by server which register the UDP channel and use it to send 
future unreliable communication to this client. The UDP channel is registered only on the
first ping, ensuring they cannot be changed by a rogue client after that.

Server verify UDP and TCP sources to ensure they match and unreliable channel is not 
polluted by rogue clients.

## Disconnection protocol

Disconnection is done by one of the sides if something is wrong in the communication
protocol (header/body decoding, unneeded packets...).

### Server disconnection protocol

- Close TCP socket
- Notify ServerThread session_id is now terminated
- Unregister network session from ServerConnection

### Client disconnection protocol

- Stop UDP ping thread
- Close TCP & UDP socket

## TCP communications

### Header
TCP application packet header is composed of:
- u32 PROTOCOL_ID
- u32 payload_size
- u16 NetworkPacket command ID
- blob NetworkPacket content

# UDP communications

### Header
UDP application packet header is composed of:
- u32 PROTOCOL_ID
- u8 udp command
- u64 user session ID
 
 ### UDP commands

- PING (1): sent regularly by client to expose its UDP channel to server
- DATA (2): Networkpacket objects over UDP. DATA payload has the following fields
  - u32 payload size
  - u16 NetworkPacket command ID
  - blob NetworkPacket content
