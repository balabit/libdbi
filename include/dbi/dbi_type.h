/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001, Brentwood Linux Users and Evangelists (BLUE).
 * Copyright (C) David Parker and Mark Tobenkin.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef unsigned short dbi_field_type_t;

typedef union dbi_variable_u {
	signed char sc;
	unsigned char uc;
	
	signed short ss;
	unsigned short us;
	
	signed long sl;
	unsigned long ul;

	float f;
	double d;
	
	short long long sll;
	unsigned long long ull;

	long double ld;
	
	char *pc;
} dbi_variable_t;

typedef struct dbi_type_s {
	dbi_field_type_t type;
	dbi_field_type_t attbs;
	dbi_variable_t data;
} dbi_type_t;

signed char dbi_schar
unsigned char dbi_uchar
signed short dbi_sshort
unsigned short dbi_ushort
signed long dbi_slong
unsigned long dbi_ulong
float dbi_float
double dbi_double
unsigned long dbi_ulong
signed long long dbi_slonglong
unsigned long long dbi_ulonglong
long double dbi_ldouble
const char *dbi_pchar



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_H__ */
