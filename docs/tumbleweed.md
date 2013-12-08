tumbleweed(1) -- Instant webserver
==================================

## SYNOPSIS

`tumbleweed` [-d] [-q] [-6] [-w workdir] [-p port] [-i index]

## DESCRIPTION

**tumbleweed** is a simple webserver for static content with a low memory footprint
for the Linux OS. It uses the Linux EPOLL event system, does non-blocking IO,
supports Zero-Copy and uses all the cores you have. It is actually really fast.
It has only a limited HTTP feature set though.

## OPTIONS

  * `-w` *workdir*:
    Share this directory. (Default: ~/Public)

  * `-p` *port*:
	Listen to this port. (Default: TCP/8080)

  * `-i` *index*:
	Serve this file if the requested entity is a directory. (Default: index.html)

  * `-d`:
	Start as a daemon and run in background. The output will be send to syslog.

  * `-q`:
	Be quiet and do not log anything.

  * `-6`:
	IPv6 only.

## EXAMPLES

Serve *~/Public* at port *8080*:

	$ tumbleweed

## BUGS

Not being fully blown with funky features.
