/*
Copyright 2011 Aiko Barz

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

#ifndef NODE_IP_H
#define NODE_IP_H

#include "main.h"

int ip_is_localhost( IP *from );
int ip_is_linklocal( IP *from );

UCHAR *ip_bytes_to_sin( IP *sin, UCHAR *p );
UCHAR *ip_sin_to_bytes( IP *sin, UCHAR *p );

void ip_merge_port_to_sin( IP *sin, int port );

#endif /* NODE_IP_H */
