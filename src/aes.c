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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <polarssl/sha2.h>
#include <polarssl/aes.h>

#include "main.h"
#include "malloc.h"
#include "log.h"
#include "aes.h"
#include "str.h"

struct obj_str *aes_encrypt(UCHAR *plain, int plainlen, UCHAR *iv, int ivlen, UCHAR *key, int keylen ) {
	UCHAR ciphertext[AES_PLAINSIZE];
	UCHAR digest[32];
	UCHAR plain_padded[AES_PLAINSIZE];
	UCHAR iv_local[AES_SALT_SIZE];
	aes_context aes_ctx;
	sha2_context sha_ctx;
	int plainlen_padded = 0;
	int i = 0;
	struct obj_str *cipher = NULL;

	if( ivlen != AES_SALT_SIZE ) {
		log_info("aes2_encrypt: Broken salt");
		return NULL;
	}

	/* Plaintext out of boundary */
	if( plainlen <= 0 || plainlen > AES_PLAINSIZE ) {
		log_info("aes2_encrypt: Broken plaintext");
		return NULL;
	}

	/* Compute padded plaintext length */
	i = plainlen % 16;
	plainlen_padded = (i == 0 ) ? plainlen : plainlen - i + 16;

	/* Plaintext out of boundary */
	if( plainlen_padded <= 0 || plainlen_padded > AES_PLAINSIZE ) {
		log_info("aes2_encrypt: Broken plaintext");
		return NULL;
	}

	/* Store padded message */
	memset(plain_padded, '\0', AES_PLAINSIZE);
	memcpy(plain_padded, plain, plainlen);

	/* Store iv locally because it gets modified by aes_crypt_cbc() */
	memcpy(iv_local, iv, ivlen);

	/* Setup AES context with the key and the IV */
	memset( digest, '\0',  32 );
	memcpy( digest, iv_local, ivlen );
	for( i = 0; i < 8192; i++ ) {
		sha2_starts( &sha_ctx, 0 );
		sha2_update( &sha_ctx, digest, 32 );
		sha2_update( &sha_ctx, key, keylen );
		sha2_finish( &sha_ctx, digest );
	}
	if( aes_setkey_enc( &aes_ctx, digest, 256 ) != 0 ) {
		log_info("aes_setkey_enc() failed");
		return NULL;
	}

	/* Encrypt message */
	if( aes_crypt_cbc( &aes_ctx, AES_ENCRYPT, plainlen_padded, iv_local, plain_padded, ciphertext ) != 0 ) {
		log_info("aes_crypt_cbc() failed");
		return NULL;
	}
	cipher = str_init(ciphertext, plainlen_padded);

	return cipher;
}

struct obj_str *aes_decrypt(UCHAR *cipher, int cipherlen, UCHAR *iv, int ivlen, UCHAR *key, int keylen ) {
	UCHAR plaintext[AES_PLAINSIZE];
	UCHAR digest[32];
	aes_context aes_ctx;
	sha2_context sha_ctx;
	int i = 0;
	struct obj_str *plain = NULL;

	if( ivlen != AES_SALT_SIZE ) {
		log_info("aes2_decrypt: Broken salt");
		return NULL;
	}

	if( cipherlen % 16 != 0 ) {
		log_info("aes2_decrypt: Broken cipher");
		return NULL;
	}

	/* Setup AES context with the key and the IV */
	memset( digest, '\0',  32 );
	memcpy( digest, iv, ivlen );
	for( i = 0; i < 8192; i++ ) {
		sha2_starts( &sha_ctx, 0 );
		sha2_update( &sha_ctx, digest, 32 );
		sha2_update( &sha_ctx, key, keylen );
		sha2_finish( &sha_ctx, digest );
	}
	if( aes_setkey_dec( &aes_ctx, digest, 256 ) != 0 ) {
		log_info("aes_setkey_enc() failed");
		return NULL;
	}

	/* Decrypt message */
	memset(plaintext, '\0', AES_PLAINSIZE);
	if( aes_crypt_cbc( &aes_ctx, AES_DECRYPT, cipherlen, iv, cipher, plaintext ) != 0 ) {
		log_info("aes_crypt_cbc() failed");
		return NULL;
	}
	plain = str_init(plaintext, cipherlen);

	return plain;
}
