tumbleweed(1) -- Instant webserver
==================================

## SYNOPSIS

`tumbleweed` [-f] [-q] [-p port] [-i index] workdir

## DESCRIPTION

**tumbleweed** is a simple webserver for static content with a low memory
footprint for the Linux OS. It uses the Linux EPOLL event system, does
non-blocking IO, supports Zero-Copy and uses all the cores you have. It is
actually really fast. It has a limited HTTP feature set though. Range
requests, even multi range requests, are supported. Pipelined HTTP requests
are supported as well. On the fly compression is not supported.

## OPTIONS

  * `-p` *port*:
	Listen to this port. (Default: TCP/8080)

  * `-i` *index*:
	Serve this file if the requested entity is a directory. (Default: index.html)

  * `-d`:
	Fork a daemon and run in background. The output will be send to syslog.

  * `-q`:
	Be quiet and do not log anything.

## EXAMPLES

Serve *~/Public* at port *8080*:

	$ tumbleweed

Serve a jekyll generated site at port *80*. tumbleweed will set its UID to
nobody when started with root privileges.

	$ sudo -E tumbleweed -p 80 /var/www/jekyll/_site

## INSTALLATION

There is a simple installation helper for Debian/Ubuntu. Just run one of the
following commands to create a installable package.

	$ make debian
	$ make ubuntu

Otherwise, you may use

	$ make
	$ sudo make install

## BUGS

Not being fully blown with funky features.
