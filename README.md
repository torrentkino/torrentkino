torrentkino(1) -- Kademlia DHT
==============================

## SYNOPSIS

`tk[46]` [-p port] [-r realm] [-d port] [-a port] [-x server] [-y port] [-q] [-l] [-s] hostname

## DESCRIPTION

**Torrentkino** is a Bittorrent DNS resolver. All DNS queries to Torrentkino get
translated into SHA1 hashes and are getting resolved by looking these up in a
Kademlia distributed hash table. It is fully compatible to the DHT as used in
most Bittorrent clients. The swarm becomes the DNS backend for Torrentkino.

By default, Torrentkino sends the first packet to a multicast address. So, for
intranet use cases, you do not need a Bittorrent bootstrap server. Just start
Torrentkino on 2 nodes without any parameters. It simply works.

If you would like to connect nodes around the globe, you may use the Bittorrent
network. Simply select a Bittorrent bootstrap server as seen in the example
below. Your client becomes a full member of the swarm and resolves Bittorrent
SHA1 hashes to IP/port tuples. The swarm on the other end does the same for you.
But in your case, the SHA1 hash represents a hostname instead of a torrent file.

Torrentkino runs as user *nobody* when started with root priviledges.

## OPTIONS

  * `-p` *port*:
	Listen to this port and use it for the DHT operations. (Default: UDP/6881)

  * `-P` *port*:
	Listen to this port and use it for the DNS operations. (Default: UDP/5353)

  * `-a` *port*:
	Announce this port (Default: UDP/8080)

  * `-n` *node id string*:
	By default a random node id gets computed on every startup. For testing
	purposes it may be useful to keep the same node id all the time. The above
	string is not used directly. Instead its SHA1 hash is used.

  * `-r` *realm*:
	Creating a realm affects the method to compute the "SHA1" hash. It helps
	you to isolate your nodes and be part of a bigger swarm at the same time.
	This is useful to handle duplicate hostnames. With different realms
	everybody may have his own https://owncloud.p2p for example.
	Technically, the realm is a SHA1 hash too. It gets merged to the hostname's
	SHA1 hash by using XOR.

  * `-x` *server*:
	Use server as a bootstrap server. Otherwise a multicast address is used.
	For LAN only cases the multicast bootstrap is enough.

  * `-y` *port*:
	The bootstrap server will be addressed at this port. (Default: UDP/6881)

  * `-l`:
	Lazy mode: This option sets multiple predefined bootstrap server like
	*router.utorrent.com* for example.

  * `-d`:
	Fork and become a daemon.

  * `-q`:
	Be quiet.

## EXAMPLES

Announce the hostnames *owncloud.p2p* and *\_http.\_tcp.foo.bar* globally.

	$ sudo tk6 -P 53 -l owncloud.p2p _http._tcp.foo.bar
	$ dig AAAA owncloud.p2p @localhost
	$ dig SRV _http._tcp.foo.bar @localhost

Announce the hostname *mycloud.p2p* within the LAN.

	$ sudo tk4 -P 53 mycloud.p2p
	$ dig mycloud.p2p @localhost

Isolate your nodes within a realm *darkness*, fork the process into background
and be quiet.

	$ sudo tk6 -r darkness -l -P 53 -d -q torrentkino.cloud
	$ dig torrentkino.cloud @localhost

## INSTALLATION

There is a simple installation helper for Debian/Ubuntu. Just run one of the
following commands to create a installable package.

	$ make debian
	$ make ubuntu

Otherwise, you may use

	$ make
	$ sudo make install

## CREDITS

Thanks to Moritz Warning for his help with the DNS API.
