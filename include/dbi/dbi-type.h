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

#ifndef __DBI_TYPE_H__
#define __DBI_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* opaque type definitions */
typedef void * dbi_driver;
typedef void * dbi_conn;
typedef void * dbi_result;

/* values for the int in field_types[] */
#define DBI_TYPE_INTEGER 1
#define DBI_TYPE_DECIMAL 2
#define DBI_TYPE_STRING 3
#define DBI_TYPE_BINARY 4
#define DBI_TYPE_ENUM 5
#define DBI_TYPE_SET 6
#define DBI_TYPE_DATETIME 7

/* values for the bitmask in field_type_attributes[] */
#define DBI_INTEGER_UNSIGNED 1
#define DBI_INTEGER_SIZE1 2
#define DBI_INTEGER_SIZE2 4
#define DBI_INTEGER_SIZE3 8
#define DBI_INTEGER_SIZE4 16
#define DBI_INTEGER_SIZE8 32
#define DBI_DECIMAL_UNSIGNED 1
#define DBI_DECIMAL_SIZE4 2
#define DBI_DECIMAL_SIZE8 4
#define DBI_STRING_FIXEDSIZE 1 /* XXX unused as of now */
#define DBI_DATETIME_DATE 1
#define DBI_DATETIME_TIME 2



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_TYPE_H__ */
