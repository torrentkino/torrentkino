torrentkino(1) -- Kademlia DHT
==============================

## SYNOPSIS

`torrentkino` [-q] [-p port] [-a hostname] [-d domain] [-r realm] [-s] [-l] [-x server] [-y port]

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

A NSS module makes any hostname with a *.p2p* TLD transparently available to
your Linux OS.

## FILES

  * **/etc/nsswitch.conf**:
	Add **tk** to the *hosts* line and your Linux OS will forward *.p2p*
	requests to the Torrentkino DHT daemon.

  * **$HOME/.torrentkino.conf**:
	This file gets written by the Torrentkino daemon and contains the server
	port number and some other hints like the TLD it shall be responsible for.
	Those hints are used by **libnss_tk.so.2** 	and the **tk** cli program.

  * **/etc/torrentkino.conf**:
	This file gets written by the Torrentkino daemon when started by root for
	example at boot time or by using sudo. It works like the file above.
	**libnss_tk.so.2** and the **tk** cli program will look for
	**$HOME/.torrentkino.conf** first.

## OPTIONS

  * `-a` *hostname*:
	Announce this hostname. By default /etc/hostname is used to determine your
	hostname. The SHA1 hash of the hostname becomes the announced info_hash.

  * `-d` *domain*:
	Instead of using the default TLD *.p2p*, you may chose a differrent TLD like
	*.darknet*, *.cloud* or *.files*. This setting primarely affects
	the NSS module, because it needs to know for what TLD it shall be
	responsible.

  * `-n` *node id string*:
	By default a random node id gets computed on every startup. For testing
	purposes it may be useful to keep the same node id all the time. The above
	string is not used directly. Instead its SHA1 hash is used.

  * `-r` *realm*:
	Creating a realm affects the method how to compute the info hash. It helps
	you to isolate your nodes and be part of a bigger swarm at the same time.
	This is useful to handle duplicate hostnames. With different realms
	everybody may have his own http://mycloud.p2p for example.

  * `-s`:
	Strict mode: In this mode all friendly nodes must operate on the same port
	to find each other. Nodes, that operate/announce different ports, do not
	show up in search results, even if they announce the *right* SHA1 hash.

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

  * `-f`:
	Fork and become a daemon.

  * `-q`:
	Be quiet.

## EXAMPLES

Announce the hostname *my.cloud* globally.

	$ torrentkino6 -a my -d cloud -l
	$ getent hosts my.cloud
	$ tk my.cloud
	$ tk http://my.cloud/index.html

Announce the hostname *mycloud.p2p* within the LAN.

	$ torrentkino4 -a mycloud
	$ getent hosts mycloud.p2p
	$ tk mycloud.p2p
	$ tk http://mycloud.p2p/index.html

Isolate your nodes within a realm *darkness*, fork the process into background
and log everything to syslog.

	$ torrentkino6 -a torrentkino -d cloud -r darkness -l -s -f -v
	$ getent hosts torrentkino.cloud
	$ tk torrentkino.cloud
	$ tk http://torrentkino.cloud/index.html

## INSTALLATION

There is a simple installation helper for Debian/Ubuntu. Just run one of the
following commands to create a installable package.

	$ make debian
	$ make ubuntu

Otherwise, you may use

	$ make
	$ sudo make install

Add **tk** to the */etc/nsswitch.conf*. Example:

	hosts: files tk dns

## SEE ALSO

nsswitch.conf(5)
