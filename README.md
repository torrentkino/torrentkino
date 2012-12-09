masala(1) -- P2P name resolution daemon
=======================================

## SYNOPSIS

`masala`  [-d] [-q] [-h hostname] [-k password] [-p port] [-x server] [-y port] [-u username]

## DESCRIPTION

**masala** is a P2P name resolution daemon. It organizes IPv6 addresses in a
distributed hash table. The basic design has been inspired by the Kademlia DHT.
The communication between nodes is realized by using bencode encoded messages on
top of UDP. There are 4 message types: PING, FIND, LOOKUP and ANNOUNCE. It is
possible to encrypt the traffic with a PSK to isolate your nodes from a bigger
swarm. By default, masala sends the first packet to a multicast address. So,
there is no configuration necessary within your broadcast domain (LAN). With a
bootstrap server, it is also possible to connect nodes around the globe. A NSS
module makes any hostname with .p2p at the end transparently available to your
Linux OS.

## FILES

**/etc/nsswitch.conf**
masala gets attached to the *hosts* line. The masala daemon is used to lookup
an IPv6 address if the requested hostname ends on **p2p**.

## OPTIONS

  * `-h` *hostname*:
    By default /etc/hostname is used to determine the hostname.

  * `-k` *password*:
	Setting a password results in encrypting each packet with AES256. The encrypted packet is encapsulated in bencode.

  * `-p` *port*:
	Listen to this port (Default: UDP/8337)

  * `-x` *server*:
	Use server as a bootstrap server. The server can be an IPv6 address, a FQHN like www.example.net or even a IPv6 multicast address. (Default: ff0e::1)

  * `-y` *port*:
	The bootstrap server will be addressed at this port. (Default: UDP/8337)

  * `-u` *username*:
    When starting as root, use -u to change the UID.

  * `-d`:
	Start as a daemon and run in background. The output will be redirected to syslog.

  * `-q`:
	Be quiet and do not log anything.

## EXAMPLES

Announce the hostname *fubar.p2p* with the encryption password *fubar*:

	$ masala -h fubar.p2p -k fubar

## BUGS

Lack of IPv6 support by the providers.

## SEE ALSO

nsswitch.conf(5)
