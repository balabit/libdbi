/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, David Parker and Mark Tobenkin.
 * http://libdbi.sourceforge.net
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * $Id$
 */

#ifndef __DBI_H__
#define __DBI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>

#include "dbi-driver.h"
#include "dbi-conn.h"
#include "dbi-result.h"

int dbi_initialize(const char *driverdir);
void dbi_shutdown();
const char *dbi_version();


/*
int dbi_result_bind_char_idx(dbi_result Result, unsigned int idx, char *bindto);
int dbi_result_bind_uchar_idx(dbi_result Result, unsigned int idx, unsigned char *bindto);
int dbi_result_bind_short_idx(dbi_result Result, unsigned int idx, short *bindto);
int dbi_result_bind_ushort_idx(dbi_result Result, unsigned int idx, unsigned short *bindto);
int dbi_result_bind_long_idx(dbi_result Result, unsigned int idx, long *bindto);
int dbi_result_bind_ulong_idx(dbi_result Result, unsigned int idx, unsigned long *bindto);
int dbi_result_bind_longlong_idx(dbi_result Result, unsigned int idx, long long *bindto);
int dbi_result_bind_ulonglong_idx(dbi_result Result, unsigned int idx, unsigned long long *bindto);

int dbi_result_bind_float_idx(dbi_result Result, unsigned int idx, float *bindto);
int dbi_result_bind_double_idx(dbi_result Result, unsigned int idx, double *bindto);

int dbi_result_bind_string_idx(dbi_result Result, unsigned int idx, const char **bindto);
int dbi_result_bind_binary_idx(dbi_result Result, unsigned int idx, const unsigned char **bindto);

int dbi_result_bind_string_copy_idx(dbi_result Result, unsigned int idx, char **bindto);
int dbi_result_bind_binary_copy_idx(dbi_result Result, unsigned int idx, unsigned char **bindto);

int dbi_result_bind_enum_idx(dbi_result Result, unsigned int idx, const char **bindto);
int dbi_result_bind_set_idx(dbi_result Result, unsigned int idx, const char **bindto);

int dbi_result_bind_datetime_idx(dbi_result Result, unsigned int idx, time_t *bindto);
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_H__ */
