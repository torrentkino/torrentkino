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
#include <openssl/evp.h>
#include <signal.h>
#include "main.h"
#include "malloc.h"
#include "aes.h"
#include "str.h"

struct obj_str *aes_encrypt(UCHAR *plain, int plainlen, UCHAR *salt, int saltlen, UCHAR *key, int keylen ) {
	EVP_CIPHER_CTX ctx;
	UCHAR rawkey[AES_KEY_SIZE];
	UCHAR iv[AES_KEY_SIZE];
	UCHAR *ciphertext = NULL;
	int ciphermax = 0;
	int cipherlen = 0;
	int tmplen = 0;
	struct obj_str *cipher = NULL;

	if ( saltlen != 8 ) {
		goto EXIT;
	}

	/* Size: plainlen + AES_BLOCK_SIZE - 1 */
	ciphermax = plainlen + AES_BLOCK_SIZE;
	ciphertext = myalloc(ciphermax * sizeof(char), "aes_encrypt");
	
	if ( EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key, keylen, AES_KEY_ROUNDS, rawkey, iv) != AES_KEY_SIZE ) {
		goto EXIT;
	}

	EVP_CIPHER_CTX_init(&ctx);
	
	if ( !EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, rawkey, iv) ) {
		goto EXIT;
	}
	if ( !EVP_EncryptInit_ex(&ctx, NULL, NULL, NULL, NULL) ) {
		goto EXIT;
	}
	if ( !EVP_EncryptUpdate(&ctx, ciphertext, &cipherlen, plain, plainlen) ) {
		goto EXIT;
	}
	if ( !EVP_EncryptFinal_ex(&ctx, ciphertext+cipherlen, &tmplen) ) {
		goto EXIT;
	}
	cipherlen += tmplen;
	if ( !EVP_CIPHER_CTX_cleanup(&ctx) ) {
		goto EXIT;
	}

	cipher = str_init(ciphertext, cipherlen);
	myfree(ciphertext, "aes_encrypt");
	return cipher;

	EXIT:
	myfree(ciphertext, "aes_encrypt");
	return NULL;
}

struct obj_str *aes_decrypt(UCHAR *cipher, int cipherlen, UCHAR *salt, int saltlen, UCHAR *key, int keylen ) {
	EVP_CIPHER_CTX ctx;
	UCHAR rawkey[AES_KEY_SIZE];
	UCHAR iv[AES_KEY_SIZE];
	UCHAR *plaintext = NULL;
	int plainmax = 0;
	int plainlen = 0;
	int tmplen = 0;
	struct obj_str *plain = NULL;

	if ( saltlen != 8 ) {
		goto EXIT;
	}

	/* Size <= cipherlen */
	plainmax = cipherlen;
	plaintext = myalloc(plainmax * sizeof(char), "aes_decrypt");

	if ( EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key, keylen, AES_KEY_ROUNDS, rawkey, iv) != AES_KEY_SIZE ) {
		goto EXIT;
	}

	EVP_CIPHER_CTX_init(&ctx);
	if ( !EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, rawkey, iv) ) {
		goto EXIT;
	}
	if ( !EVP_DecryptInit_ex(&ctx, NULL, NULL, NULL, NULL) ) {
		goto EXIT;
	}
	if ( !EVP_DecryptUpdate(&ctx, plaintext, &plainlen, cipher, cipherlen) ) {
		goto EXIT;
	}
	if ( !EVP_DecryptFinal_ex(&ctx, plaintext+plainlen, &tmplen) ) {
		goto EXIT;
	}
	plainlen += tmplen;
	if ( !EVP_CIPHER_CTX_cleanup(&ctx) ) {
		goto EXIT;
	}

	plain = str_init(plaintext, plainlen);
	myfree(plaintext, "aes_decrypt");
	return plain;

	EXIT:
	myfree(plaintext, "aes_decrypt");
	return NULL;
}
