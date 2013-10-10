tumbleweed(1) -- Instant webserver
==================================

## SYNOPSIS

`tumbleweed` [-d] [-q] [-6] [-w workdir] [-p port] [-i index] [-u username]

## DESCRIPTION

**tumbleweed** is a simple webserver for static content with a low memory footprint
for the Linux OS. It uses the Linux EPOLL event system, does non-blocking IO,
supports Zero-Copy and uses all the cores you have. It is actually quite fast.
Though only a few HTTP features are supported such as HTTP Pipelining,
Keep-alive and Range Requests as seen in the wild.

## OPTIONS

  * `-w` *workdir*:
    Share this directory. (Default: ~/Public)

  * `-p` *port*:
	Listen to this port. (Default: TCP/8080)

  * `-i` *index*:
	Serve this file if the requested entity is a directory. (Default: index.html)

  * `-u` *username*:
    When starting as root, use -u to change the UID.

  * `-d`:
	Start as a daemon and run in background. The output will be send to syslog.

  * `-q`:
	Be quiet and do not log anything.

  * `-6`:
	IPv6 only.

## INSTALLATION
  * apt-get install build-essential
  * apt-get install debhelper
  * apt-get install git-buildpackage
  * apt-get install libmagic-dev
  * git-buildpackage

## EXAMPLES

Serve *~/Public* at port *8080*:

	$ tumbleweed

## BUGS

Not being fully blown with funky features.
