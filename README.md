torrentkino(1) -- Kademlia DHT
==============================

## SYNOPSIS

`tk[46]` [-a hostname] [-b port] [-r realm] [-p port] [-x server] [-y port] [-q] [-l] [-s]

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

## OPTIONS

  * `-a` *hostname*:
	Announce this hostname. By default /etc/hostname is used to determine your
	hostname. The SHA1 hash of the hostname becomes the announced info_hash.

  * `-b` *port*:
	Announce this port together with your hostname. (Default: "8080")

  * `-n` *node id string*:
	By default a random node id gets computed on every startup. For testing
	purposes it may be useful to keep the same node id all the time. The above
	string is not used directly. Instead its SHA1 hash is used.

  * `-r` *realm*:
	Creating a realm affects the method how to compute the info hash. It helps
	you to isolate your nodes and be part of a bigger swarm at the same time.
	This is useful to handle duplicate hostnames. With different realms
	everybody may have his own http://mycloud.p2p for example.

  * `-p` *port*:
	Listen to this port (Default: UDP/6881)

  * `-x` *server*:
	Use server as a bootstrap server. Otherwise a multicast address is used.
	For LAN only cases the multicast bootstrap is enough.

  * `-y` *port*:
	The bootstrap server will be addressed at this port. (Default: UDP/6881)

  * `-l`:
	Lazy mode: This option sets a predefined bootstrap server like
	*router.utorrent.com* for example.

  * `-s`:
	Strict mode: In this mode all friendly nodes must operate on the same port
	to find each other. Nodes, that operate/announce different ports, do not
	show up in search results, even if they announce the *right* SHA1 hash.

  * `-f`:
	Fork and become a daemon.

  * `-q`:
	Be quiet.

## EXAMPLES

Announce the hostname *my.cloud* globally.

	$ tk6 -a my.cloud -l
	$ host my.cloud ::1

Announce the hostname *mycloud.p2p* within the LAN.

	$ tk4 -a mycloud.p2p
	$ host mycloud.p2p 127.0.0.1

Isolate your nodes within a realm *darkness*, fork the process into background
and log everything to syslog.

	$ tk6 -a torrentkino.cloud -r darkness -l -s -f -v
	$ host torrentkino.cloud ::1

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
