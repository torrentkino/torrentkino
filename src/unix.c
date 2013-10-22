/*
Copyright 2006 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pwd.h>

#include "unix.h"

void unix_signal( struct sigaction *sig_stop, struct sigaction *sig_time ) {
	/* STRG+C aka SIGINT => Stop the program */
	sig_stop->sa_handler = unix_sig_stop;
	sig_stop->sa_flags = 0;
	if( ( sigemptyset( &sig_stop->sa_mask ) == -1 ) ||( sigaction( SIGINT, sig_stop, NULL ) != 0 ) ) {
		fail( "Failed to set SIGINT to handle Ctrl-C" );
	}

	/* ALARM */
	sig_time->sa_handler = unix_sig_time;
	sig_time->sa_flags = 0;
	if( ( sigemptyset( &sig_time->sa_mask ) == -1 ) ||( sigaction( SIGALRM, sig_time, NULL ) != 0 ) ) {
		fail( "Failed to set SIGALRM to handle Timeouts" );
	}

	/* Ignore broken PIPE. Otherwise, the server dies too whenever a browser crashes. */
	signal( SIGPIPE, SIG_IGN );
}

void unix_sig_stop( int signo ) {
	status = GAMEOVER;
}

void unix_sig_time( int signo ) {
	status = GAMEOVER;
}

void unix_set_time( int seconds ) {
	alarm( seconds );
}

void unix_fork( void ) {
	pid_t pid = 0;

	pid = fork();
	if( pid < 0 ) {
		fail( "fork() failed" );
	} else if( pid != 0 ) {
	   exit( 0 );
	}

	/* Become session leader */
	setsid();

	/* Clear out the file mode creation mask */
	umask( 0 );
}

void unix_limits( int cores, int max_events ) {
	struct rlimit rl;
	int guess = 2 * max_events * cores + 50;
	int limit = (guess < 4096 ) ? 4096 : guess; /* RLIM_INFINITY; */

	if( getuid() != 0 ) {
		return;
	}

	rl.rlim_cur = limit;
	rl.rlim_max = limit;

	if( setrlimit( RLIMIT_NOFILE, &rl ) == -1 ) {
		fail( strerror( errno ) );
	}

	info( NULL, 0, "Max open files: %i", limit );
}

void unix_dropuid0( void ) {
	struct passwd *pw = NULL;
	
	if( getuid() != 0 ) {
		return;
	}

	/* Process is running as root, drop privileges */
	if( ( pw = getpwnam( CONF_USERNAME ) ) == NULL ) {
		fail( "Dropping uid 0 failed." );
	}
	if( setenv( "HOME", pw->pw_dir, 1 ) != 0 ) {
		fail( "setenv: Setting new $HOME failed." );
	}
	if( setgid( pw->pw_gid ) != 0 ) {
		fail( "setgid: Unable to drop group privileges" );
	}
	if( setuid( pw->pw_uid ) != 0 ) {
		fail( "setuid: Unable to drop user privileges" );
	}

	/* Test permissions */
	if( setuid( 0 ) != -1 ) {
		fail( "ERROR: Managed to regain root privileges?" );
	}
	if( setgid( 0 ) != -1 ) {
		fail( "ERROR: Managed to regain root privileges?" );
	}

	info( NULL, 0, "uid: %i, gid: %i", pw->pw_uid, pw->pw_gid );
}

#if 0
void unix_environment( void ) {
	char buffer[BUF_SIZE];
#ifdef __i386__
	snprintf( buffer, BUF_SIZE, "Types: int: %i, long int: %i, size_t: %i, ssize_t: %i, time_t: %i", sizeof(int), sizeof(long int), sizeof(size_t), sizeof(ssize_t), sizeof(time_t) );
#else
	snprintf( buffer, BUF_SIZE, "Types: int: %lu, long int: %lu, size_t: %lu, ssize_t: %lu, time_t: %lu", sizeof(int), sizeof(long int), sizeof(size_t), sizeof(ssize_t), sizeof(time_t) );
#endif
	log_simple( buffer );
}
#endif

int unix_cpus( void ) {
	return sysconf( _SC_NPROCESSORS_ONLN );
}
