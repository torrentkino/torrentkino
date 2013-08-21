/*
Copyright 2012 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <polarssl/sha2.h>
#include <polarssl/aes.h>

#include "aes.h"

struct obj_str *aes_encrypt( UCHAR *plain, int plainlen, UCHAR *iv, char *key, int keylen ) {
	UCHAR ciphertext[AES_MSG_SIZE];
	UCHAR digest[AES_KEY_SIZE];
	UCHAR plain_padded[AES_MSG_SIZE];
	UCHAR iv_local[AES_IV_SIZE];
	aes_context aes_ctx;
	int plainlen_padded = 0;
	int i = 0;
	struct obj_str *cipher = NULL;

	/* Plaintext out of boundary */
	if( plainlen <= 0 || plainlen > AES_MSG_SIZE ) {
		info( NULL, 0, "aes2_encrypt: Broken plaintext" );
		return NULL;
	}

	/* Compute padded plaintext length */
	i = plainlen % AES_BLOCK_SIZE;
	plainlen_padded = (i == 0 ) ? plainlen : plainlen - i + AES_BLOCK_SIZE;

	/* Plaintext out of boundary */
	if( plainlen_padded <= 0 || plainlen_padded > AES_MSG_SIZE ) {
		info( NULL, 0, "aes2_encrypt: Broken plaintext" );
		return NULL;
	}

	/* Store padded message */
	memset( plain_padded, '\0', AES_MSG_SIZE );
	memcpy( plain_padded, plain, plainlen );

	/* Store iv locally because it gets modified by aes_crypt_cbc() later */
	memcpy( iv_local, iv, AES_IV_SIZE );

	/* Setup AES context with the key and the IV */
	aes_key_setup( digest, iv_local, key, keylen );
	if( aes_setkey_enc( &aes_ctx, digest, 256 ) != 0 ) {
		info( NULL, 0, "aes_setkey_enc() failed" );
		return NULL;
	}

	/* Encrypt message */
	if( aes_crypt_cbc( &aes_ctx, AES_ENCRYPT, plainlen_padded, iv_local, plain_padded, ciphertext ) != 0 ) {
		info( NULL, 0, "aes_crypt_cbc() failed" );
		return NULL;
	}
	cipher = str_init( ciphertext, plainlen_padded );

	return cipher;
}

struct obj_str *aes_decrypt( UCHAR *cipher, int cipherlen, UCHAR *iv, char *key, int keylen ) {
	UCHAR plaintext[AES_MSG_SIZE];
	UCHAR digest[AES_KEY_SIZE];
	aes_context aes_ctx;
	struct obj_str *plain = NULL;

	if( cipherlen % AES_BLOCK_SIZE != 0 ) {
		info( NULL, 0, "aes2_decrypt: Broken cipher" );
		return NULL;
	}

	/* Setup AES context with the key and the IV */
	aes_key_setup( digest, iv, key, keylen );
	if( aes_setkey_dec( &aes_ctx, digest, 256 ) != 0 ) {
		info( NULL, 0, "aes_setkey_enc() failed" );
		return NULL;
	}

	/* Decrypt message */
	memset( plaintext, '\0', AES_MSG_SIZE );
	if( aes_crypt_cbc( &aes_ctx, AES_DECRYPT, cipherlen, iv, cipher, plaintext ) != 0 ) {
		info( NULL, 0, "aes_crypt_cbc() failed" );
		return NULL;
	}
	plain = str_init( plaintext, cipherlen );

	/* The decrypted message may be bigger than the original message, but
	 * the bencode parser can handle that. */
	return plain;
}

void aes_key_setup( UCHAR *digest, UCHAR *iv, char *key, int keylen ) {
	sha2_context sha_ctx;
	int i = 0;

	memset( digest, '\0', AES_KEY_SIZE );
	memcpy( digest, iv, AES_IV_SIZE );
	for( i = 0; i < AES_KEY_ROUNDS; i++ ) {
		sha2_starts( &sha_ctx, 0 );
		sha2_update( &sha_ctx, digest, AES_KEY_SIZE );
		sha2_update( &sha_ctx, (UCHAR *)key, keylen );
		sha2_finish( &sha_ctx, digest );
	}
}
