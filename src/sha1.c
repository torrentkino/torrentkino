/*
Copyright 2012 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <netinet/in.h>
#ifdef OPENSSL
#include <openssl/ssl.h>
#else
#include <polarssl/sha1.h>
#endif

#include "main.h"
#include "str.h"
#include "list.h"
#include "file.h"
#include "conf.h"
#include "malloc.h"
#include "log.h"

void sha1_hash(unsigned char *hash, const char *buffer, long int bytes ) {
	memset(hash, '\0', SHA_DIGEST_LENGTH);
#ifdef OPENSSL
	SHA1((unsigned char *)buffer, bytes, hash);
#else
	sha1((const unsigned char *)buffer, bytes, hash);
#endif
}

unsigned char *sha1_hashfile(const char *filename ) {
#ifdef OPENSSL
	SHA_CTX ctx;
	off_t off = 0;
	size_t chunk = 33554432; /* 32 MB */
	size_t size = 0;
	char *buf = NULL;
	size_t fsize = file_size(filename);
	unsigned char *md = (unsigned char *) myalloc((SHA_DIGEST_LENGTH+1)*sizeof(unsigned char), "sha1_hashfile");

	if ( !SHA1_Init(&ctx) ) {
		myfree(md, "sha1_hashfile");
		return NULL;
	}

	for ( off=0; off<fsize; off+=chunk ) {
		if ( off+chunk > fsize )
			size = fsize - off;
		else
			size = chunk;
		
		buf = file_load(filename, off, size);

		if ( buf == NULL ) {
			myfree(md, "sha1_hashfile");
			return NULL;
		}

		if ( !SHA1_Update(&ctx, buf, size) ) {
			myfree(md, "sha1_hashfile");
			myfree(buf, "sha1_hashfile");
			return NULL;
		}

		myfree(buf, "sha1_hashfile");
	}

	if ( !SHA1_Final(md, &ctx) ) {
		myfree(md, "sha1_hashfile");
		return NULL;
	}

	return md;
#else
	unsigned char *md = (unsigned char *) myalloc((SHA_DIGEST_LENGTH+1)*sizeof(unsigned char), "sha1_hashfile");
	
	if ( sha1_file(filename, md) != 0 ) {
		myfree(md, "sha1_hashfile");
		return NULL;
	}

	return md;
#endif
}
