torrentkino(1) -- Kademlia DHT
==============================

## SYNOPSIS

`torrentkino` [-d] [-q] [-h hostname] [-r realm] [-p port] [-x server] [-y port]

## DESCRIPTION

**torrentkino** is a P2P name resolution daemon. It resolves hostnames into IP
addresses by using a Kademlia distributed hash table. This DHT is compatible to
the DHT as used in Bittorrent clients like Transmission.

By default, torrentkino sends the first packet to a multicast address. So, for
intranet use cases, you do not need a bootstrap server. Just start torrentkino
on 2 nodes without any parameters. It simply works.

If you would like to connect nodes around the globe, you may use the Bittorrent
network. Simply select a Bittorrent bootstrap server as seen in the example
below. Your client becomes a full member of the swarm and resolves info hashes
to IP/port tuples. The swarm on the other end does the same for you. But in
your case, the info hash represents a hostname instead of a torrent file.

A NSS module makes any hostname with *.p2p* at the end transparently available
to your Linux OS.

Torrentkino uses a hostname cache for fast lookups within big swarms. A queried
hostname will be cached for 30 minutes. Torrentkino also lookups that queried
hostname every 5 minutes on its own. The cache keeps being up-to-date. It stops
30 minutes after your last query for that hostname.

## FILES

  * **/etc/nsswitch.conf**:
	Add **tk** to the *hosts* line and your Linux OS will forward *.p2p*
	requests to the Torrentkino DHT daemon.

  * **$HOME/.torrentkino.conf**:
    This file gets written by the Torrentkino daemon and contains the server port
	number and some other hints. Those hints are used by **libnss_tk.so.2** and
	the **tk** cli program.
  
  * **/etc/torrentkino.conf**:
	This file gets written by the Torrentkino daemon when started by root for
	example at boot time or by using sudo. It works like the file above.
	**libnss_tk.so.2** and the **tk** cli program will look for 
	**$HOME/.torrentkino.conf** first.

## OPTIONS

  * `-a` *hostname*:
    By default /etc/hostname is used to determine your hostname. The SHA1 hash
	of the hostname becomes the announced info_hash.

  * `-b` *port*:
	Kademlia also stores a port. You can use **tk** to ask for that port and
	-b to specify that port. (Default: "8080")

  * `-n` *node id string*:
    By default a random node id gets computed on every startup. For testing
	purposes it may be useful to keep the same node id all the time. The above
	string is not used directly. Instead its SHA1 hash is used.

  * `-r` *realm*:
	Creating a realm affects the method how to compute the info hash. It helps
	you to isolate your nodes and be part of a bigger swarm at the same time.
	This is useful to handle duplicate hostnames. So now, everybody may have his
	own https://owncloud.p2p within his own realm for example.

  * `-p` *port*:
	Listen to this port (Default: UDP/6881)

  * `-x` *server*:
	Use server as a bootstrap server. The server can be an IPv6 address, a FQHN
	like www.example.net or even a IPv6 multicast address. (Default: ff0e::1)

  * `-y` *port*:
	The bootstrap server will be addressed at this port. (Default: UDP/6881)

  * `-d`:
	Start as a daemon and run in background. The output will be send to syslog.

  * `-v`:
	Verbose.

  * `-k` *password*:
	Setting a password results in encrypting each packet with AES256. The
	encrypted message is encapsulated in bencode too. You effectively
	isolate your nodes from the rest of the world this way. This method is not
	compatible to the Bittorrent network and only works between torrentkino
	daemons. (Torrentkino needs to be compiled with PolarSSL support. See Makefile.)

## EXAMPLES

Announce the hostname *owncloud.p2p* within the realm *TooManySecrets* to a global
Bittorrent swarm.

	$ torrentkino6 -h owncloud.p2p -r TooManySecrets -x dht.wifi.pps.jussieu.fr
	$ getent hosts owncloud.p2p
	$ tk owncloud.p2p
	$ tk http://owncloud.p2p/index.html

	$ torrentkino4 -h owncloud.p2p -r TooManySecrets -x router.utorrent.com
	$ getent hosts owncloud.p2p
	$ tk owncloud.p2p
	$ tk http://owncloud.p2p/index.html

## INSTALLATION

There is a simple installation helper for Debian/Ubuntu. Just run one of the
following commands to create a installable package.

	$ make debian
	$ make ubuntu

Otherwise, you may use

	$ make
	$ sudo make install

## SEE ALSO

nsswitch.conf(5)
