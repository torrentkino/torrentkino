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

#ifndef AES_H
#define AES_H

#include "main.h"
#include "ben.h"

#define AES_IV_SIZE 16
#define AES_KEY_SIZE 32
#define AES_MSG_SIZE 1456
#define AES_BLOCK_SIZE 16
#define AES_KEY_ROUNDS 8

struct obj_str *aes_encrypt( UCHAR *plain, int plainlen, UCHAR *iv, char *key, int keylen );
struct obj_str *aes_decrypt( UCHAR *cipher, int cipherlen, UCHAR *iv, char *key, int keylen );
void aes_key_setup( UCHAR *digest, UCHAR *iv, char *key, int keylen );

#endif /* AES_H */
