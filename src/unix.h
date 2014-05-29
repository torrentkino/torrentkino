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

#ifndef UNIX_H
#define UNIX_H

#include "main.h"
#include "fail.h"
#include "log.h"

void unix_signal( struct sigaction *sig_stop, struct sigaction *sig_time );
void unix_sig_stop( int signo );
void unix_sig_time( int signo );
void unix_set_time( int seconds );
void unix_fork( int mode );
void unix_limits( int cores, int max_events );
void unix_dropuid0( void );
int unix_cpus( void );
/*
void unix_environment( void );
*/

#endif
