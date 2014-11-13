/*
Copyright 2012 Aiko Barz

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

#ifdef POLARSSL
#include <polarssl/sha1.h>
#endif

#include "sha1.h"

#ifdef POLARSSL
void sha1_hash( UCHAR *hash, const char *buffer, long int bytes ) {
	memset( hash, '\0', SHA1_SIZE );
	sha1( (const UCHAR *)buffer, bytes, hash );
}
#else
void sha1_hash( UCHAR *hash, const char *buffer, long int bytes ) {
	blk_SHA_CTX c;
	blk_SHA1_Init( &c );
	blk_SHA1_Update( &c, buffer, bytes );
	blk_SHA1_Final( hash, &c );
}
#endif
