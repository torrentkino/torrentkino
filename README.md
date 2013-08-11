masala(1) -- P2P name resolution daemon
=======================================

## SYNOPSIS

`masala`  [-d] [-q] [-h hostname] [-k password] [-r realm] [-p port]
[-x server] [-y port] [-u username]

## DESCRIPTION

**masala** is a P2P name resolution daemon. It resolves hostnames into IPv6
addresses by using a distributed hash table. This DHT is compatible to the
Kademlia DHT as used in Bittorrent clients like Transmission.

By default, masala sends the first packet to a multicast address. So, for
intranet use cases, you do not need a bootstrap server. Just start masala on 2
nodes without any parameters. It simply works.

If you would like to connect nodes around the globe, you may use the Bittorrent
network. Simply select a Bittorrent bootstrap server as seen in the example
below. Your client becomes a full member of the swarm and resolves info hashes
to IPv6/port tuples. The swarm on the other end does the same for you. But in
your case, the info hash represents a hostname and not a torrent file.

A NSS module makes any hostname with *.p2p* at the end transparently available
to your Linux OS. The NSS modul tries to contact the masala daemon at ::1 and
UDP/6881. The daemon then resolves the requested hostname recursively even if
you use encryption. As long as the connection comes from ::1 the masala daemon
will accept the recursive requests.

Masala uses a hostname cache for fast lookups within big swarms. A queried
hostname will be cached for 30 minutes. Masala also lookups that queried
hostname every 5 minutes on its own. So, the cache stays up-to-date. It stops
30 minutes after your last query for that hostname.

## FILES

  * **/etc/nsswitch.conf**:
	masala gets attached to the *hosts* line. masala is only used for lookup
	requests with *.p2p* as the TLD.

## OPTIONS

  * `-h` *hostname*:
    By default /etc/hostname is used to determine the hostname. The SHA1 hash of
	the hostname becomes the announced info_hash.

  * `-n` *node id string*:
    By default a random node id gets computed on every startup. For testing
	purposes it may be useful to keep the same node id all the time. The above
	string is not used directly. Instead its SHA1 hash is used.

  * `-k` *password*:
	Setting a password results in encrypting each packet with AES256. The
	encrypted message is encapsulated in bencode too. You effectively
	isolate your nodes from the rest of the world this way. This method is not
	compatible to the Bittorrent network and only works between masala
	daemons.

  * `-r` *realm*:
	Creating a realm affects the method how to compute the info hash. It helps
	you to isolate your nodes and be part of a bigger swarm at the same time.
	This is useful to handle duplicate hostnames. So now, everybody may have his
	own https://owncloud.p2p within his own realm for example.

  * `-p` *port*:
	Listen to this port (Default: UDP/6881)

  * `-a` *port*:
	Kademlia also stores a port. You can use masala-cli to ask for that port and
	-a to specify that port. (Default: "6881")

  * `-x` *server*:
	Use server as a bootstrap server. The server can be an IPv6 address, a FQHN
	like www.example.net or even a IPv6 multicast address. (Default: ff0e::1)

  * `-y` *port*:
	The bootstrap server will be addressed at this port. (Default: UDP/6881)

  * `-u` *username*:
    When starting as root, use -u to change the UID.

  * `-d`:
	Start as a daemon and run in background. The output will be send to syslog.

  * `-v`:
	Verbose.

## INSTALLATION
  * apt-get install build-essential
  * apt-get install debhelper
  * apt-get install git-buildpackage
  * apt-get install libpolarssl-dev
  * git-buildpackage
  * dpkg -i ../masala_0.6_amd64.deb

## EXAMPLES

Announce the hostname *owncloud.p2p* within the realm TooManySecrets to an IPv6
Bittorrent bootstrap server. That bootstrap server is maintained by the creator
of the Transmission DHT stack:

	$ masala -h owncloud.p2p -r TooManySecrets -x dht.wifi.pps.jussieu.fr
	$ getent hosts owncloud.p2p
	$ masala-cli owncloud.p2p

## BUGS

Lack of IPv6 support by the providers.

## SEE ALSO

nsswitch.conf(5)
