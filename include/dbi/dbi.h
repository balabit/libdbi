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

/* opaque type definitions */
#define dbi_plugin void *
#define dbi_driver void *
#define dbi_result void *

/* values for the int in field_types[] */
#define DBI_TYPE_INTEGER 1
#define DBI_TYPE_DECIMAL 2
#define DBI_TYPE_STRING 3
#define DBI_TYPE_BINARY 4
#define DBI_TYPE_ENUM 5
#define DBI_TYPE_SET 6

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
#define DBI_STRING_FIXEDSIZE 1 /* XXX */

int dbi_initialize(const char *plugindir);
void dbi_shutdown();
const char *dbi_version();

dbi_plugin dbi_plugin_list(dbi_plugin Current); /* returns next plugin. if current is NULL, return first plugin. */
dbi_plugin dbi_plugin_open(const char *name); /* goes thru linked list until it finds the right one */
int dbi_plugin_is_reserved_word(dbi_plugin Plugin, const char *word);
void *dbi_plugin_specific_function(dbi_plugin Plugin, const char *name);

const char *dbi_plugin_get_name(dbi_plugin Plugin);
const char *dbi_plugin_get_filename(dbi_plugin Plugin);
const char *dbi_plugin_get_description(dbi_plugin Plugin);
const char *dbi_plugin_get_maintainer(dbi_plugin Plugin);
const char *dbi_plugin_get_url(dbi_plugin Plugin);
const char *dbi_plugin_get_version(dbi_plugin Plugin);
const char *dbi_plugin_get_date_compiled(dbi_plugin Plugin);

dbi_driver dbi_driver_new(const char *name); /* shortcut for dbi_driver_open(dbi_plugin_open("foo")) */
dbi_driver dbi_driver_open(dbi_plugin Plugin); /* returns an actual instance of the driver */
dbi_plugin dbi_driver_get_plugin(dbi_driver Driver);
int dbi_driver_set_option(dbi_driver Driver, const char *key, char *value); /* if value is NULL, remove option from list */
int dbi_driver_set_option_numeric(dbi_driver Driver, const char *key, int value);
const char *dbi_driver_get_option(dbi_driver Driver, const char *key);
int dbi_driver_get_option_numeric(dbi_driver Driver, const char *key);
const char *dbi_driver_get_option_list(dbi_driver Driver, const char *current); /* returns key of next option, or the first option key if current is NULL */
void dbi_driver_clear_option(dbi_driver Driver, const char *key);
void dbi_driver_clear_options(dbi_driver Driver);
void dbi_driver_close(dbi_driver Driver);

int dbi_driver_error(dbi_driver Driver, char *errmsg_dest);
void dbi_driver_error_handler(dbi_driver Driver, void *function, void *user_argument);

int dbi_driver_connect(dbi_driver Driver);
dbi_result dbi_driver_get_db_list(dbi_driver Driver);
dbi_result dbi_driver_get_table_list(dbi_driver Driver, const char *db);
dbi_result dbi_driver_query(dbi_driver Driver, const char *formatstr, ...); 
int dbi_driver_select_db(dbi_driver Driver, const char *db);

dbi_driver dbi_result_get_driver(dbi_result Result);
int dbi_result_free(dbi_result Result);
int dbi_result_seek_row(dbi_result Result, unsigned int row);
int dbi_result_first_row(dbi_result Result);
int dbi_result_last_row(dbi_result Result);
int dbi_result_prev_row(dbi_result Result);
int dbi_result_next_row(dbi_result Result);
unsigned int dbi_result_get_numrows(dbi_result Result);
unsigned int dbi_result_get_numrows_affected(dbi_result Result);
unsigned int dbi_result_get_field_size(dbi_result Result, const char *fieldname);
unsigned int dbi_result_get_field_length(dbi_result Result, const char *fieldname); /* size-1 */

int dbi_result_get_fields(dbi_result Result, const char *format, ...);
int dbi_result_bind_fields(dbi_result Result, const char *format, ...);

signed char dbi_result_get_char(dbi_result Result, const char *fieldname);
unsigned char dbi_result_get_uchar(dbi_result Result, const char *fieldname);
short dbi_result_get_short(dbi_result Result, const char *fieldname);
unsigned short dbi_result_get_ushort(dbi_result Result, const char *fieldname);
long dbi_result_get_long(dbi_result Result, const char *fieldname);
unsigned long dbi_result_get_ulong(dbi_result Result, const char *fieldname);
long long dbi_result_get_longlong(dbi_result Result, const char *fieldname);
unsigned long long dbi_result_get_ulonglong(dbi_result Result, const char *fieldname);

float dbi_result_get_float(dbi_result Result, const char *fieldname);
double dbi_result_get_double(dbi_result Result, const char *fieldname);

const char *dbi_result_get_string(dbi_result Result, const char *fieldname);
const unsigned char *dbi_result_get_binary(dbi_result Result, const char *fieldname);

char *dbi_result_get_string_copy(dbi_result Result, const char *fieldname);
unsigned char *dbi_result_get_binary_copy(dbi_result Result, const char *fieldname);

const char *dbi_result_get_enum(dbi_result Result, const char *fieldname);
const char *dbi_result_get_set(dbi_result Result, const char *fieldname);

int dbi_result_bind_char(dbi_result Result, const char *fieldname, char *bindto);
int dbi_result_bind_uchar(dbi_result Result, const char *fieldname, unsigned char *bindto);
int dbi_result_bind_short(dbi_result Result, const char *fieldname, short *bindto);
int dbi_result_bind_ushort(dbi_result Result, const char *fieldname, unsigned short *bindto);
int dbi_result_bind_long(dbi_result Result, const char *fieldname, long *bindto);
int dbi_result_bind_ulong(dbi_result Result, const char *fieldname, unsigned long *bindto);
int dbi_result_bind_longlong(dbi_result Result, const char *fieldname, long long *bindto);
int dbi_result_bind_ulonglong(dbi_result Result, const char *fieldname, unsigned long long *bindto);

int dbi_result_bind_float(dbi_result Result, const char *fieldname, float *bindto);
int dbi_result_bind_double(dbi_result Result, const char *fieldname, double *bindto);

int dbi_result_bind_string(dbi_result Result, const char *fieldname, const char **bindto);
int dbi_result_bind_binary(dbi_result Result, const char *fieldname, const unsigned char **bindto);

int dbi_result_bind_string_copy(dbi_result Result, const char *fieldname, char **bindto);
int dbi_result_bind_binary_copy(dbi_result Result, const char *fieldname, unsigned char **bindto);

int dbi_result_bind_enum(dbi_result Result, const char *fieldname, const char **bindto);
int dbi_result_bind_set(dbi_result Result, const char *fieldname, const char **bindto);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_H__ */
