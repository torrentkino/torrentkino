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

#include <string.h>

#include "opts.h"

BEN *opts_init( void ) {
	return ben_init( BEN_DICT );
}

void opts_free( BEN *opts ) {
	ben_free( opts );
}

void opts_load( BEN *opts, int argc, char **argv ) {
	BEN *key = NULL;
	BEN *val = NULL;
	unsigned int i;

	/* ./program blabla */
	if( argc < 2 ) {
		return;
	}

	if( argv == NULL ) {
		return;
	}

	for( i=0; i<argc; i++ ) {
		if( argv[i] != NULL && argv[i][0] == '-' ) {
			
			/* -x */
			if( strlen( argv[i] ) == 2 ) {
				if( i+1 < argc && argv[i+1] != NULL && argv[i+1][0] != '-' ) {

					/* -x abc */
					key = ben_init( BEN_STR );
					val = ben_init( BEN_STR );
					ben_str( key, ( UCHAR *)argv[i], strlen( argv[i] ) );
					ben_str( val, ( UCHAR *)argv[i+1], strlen( argv[i+1] ) );
					ben_dict( opts, key, val );
					i++;

				} else {

					/* -x -y => -x */
					key = ben_init( BEN_STR );
					val = ben_init( BEN_STR );
					ben_str( key, ( UCHAR *)argv[i], strlen( argv[i] ) );
					ben_str( val, ( UCHAR *)"_", 1 );
					ben_dict( opts, key, val );
				}
			}
		}
	}
}
