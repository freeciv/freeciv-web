/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__MD5_H
#define FC__MD5_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MD5_BITS 128
#define MD5_HEX_BYTES (MD5_BITS / 4)
#define MD5_BIN_BYTES (MD5_BITS / 8)

#define MAX_MD5_BIN_BYTES MD5_BIN_BYTES

void create_md5sum(const unsigned char *input, int len,
                   char output[MD5_HEX_BYTES + 1]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__MD5_H */
