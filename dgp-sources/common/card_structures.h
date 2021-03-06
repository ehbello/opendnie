/*
 * card_structures.h: Support functions for additional card structures
 *
 * Copyright (C) 2006-2010 Dirección General de la Policía y de la Guardia Civil
 * oficinatecnica@dnielectronico.es
 *
 * This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef _CARD_STRUCTURES_H
#define _CARD_STRUCTURES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <opensc/opensc.h>

/* structure defintions */
struct card_pkcs15_df {
  u8 *            data;        /* Buffer to hold PKCS#15 struct info               */
  size_t          data_len;    /* Data buffer length and also the corresponding    */              
  size_t          file_len;    /* PKCS#15 card file size                           */
  size_t          filled_len;  /* Length of pkcs#15 struct info stored on df       */ 
                               /* This value will be lower or at maximum           */
                               /*  equals than len and will be increased/decreased */ 
                               /*  when added/deleted pkcs#15 objects              */
  sc_path_t       df_path;     /* Path of pkcs15_df                                */                               
  unsigned int    type;        /* PKCS#15 DF type                                  */
};
typedef struct card_pkcs15_df card_pkcs15_df_t;

#ifdef __cplusplus
}
#endif

#endif /* _CARD_STRUCTURES_H */
