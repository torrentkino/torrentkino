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

#define AES_BLOCK_SIZE 256
#define AES_SALT_SIZE 16
#define AES_KEY_SIZE 32
#define AES_KEY_ROUNDS 5

#define AES_PLAINSIZE 1456

struct obj_str *aes_encrypt( UCHAR *plain, int plainlen, UCHAR *salt, int saltlen, UCHAR *key, int keylen );
struct obj_str *aes_decrypt( UCHAR *cipher, int cipherlen, UCHAR *salt, int saltlen, UCHAR *key, int keylen );
