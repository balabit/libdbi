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

#ifndef __DBI_CONN_H__
#define __DBI_CONN_H__

#ifdef __cplusplus
extern "C" {
#endif

dbi_conn dbi_conn_new(const char *name); /* shortcut for dbi_conn_open(dbi_driver_open("foo")) */
dbi_conn dbi_conn_open(dbi_driver Driver); /* returns an actual instance of the conn */
dbi_driver dbi_conn_get_driver(dbi_conn Conn);
int dbi_conn_set_option(dbi_conn Conn, const char *key, char *value); /* if value is NULL, remove option from list */
int dbi_conn_set_option_numeric(dbi_conn Conn, const char *key, int value);
const char *dbi_conn_get_option(dbi_conn Conn, const char *key);
int dbi_conn_get_option_numeric(dbi_conn Conn, const char *key);
const char *dbi_conn_get_option_list(dbi_conn Conn, const char *current); /* returns key of next option, or the first option key if current is NULL */
void dbi_conn_clear_option(dbi_conn Conn, const char *key);
void dbi_conn_clear_options(dbi_conn Conn);
void dbi_conn_close(dbi_conn Conn);

int dbi_conn_error(dbi_conn Conn, char **errmsg_dest);
void dbi_conn_error_handler(dbi_conn Conn, void *function, void *user_argument);

int dbi_conn_connect(dbi_conn Conn);
int dbi_conn_get_socket(dbi_conn Conn);
dbi_result dbi_conn_get_db_list(dbi_conn Conn, const char *pattern);
dbi_result dbi_conn_get_table_list(dbi_conn Conn, const char *db);
dbi_result dbi_conn_query(dbi_conn Conn, const char *formatstr, ...); 
int dbi_conn_select_db(dbi_conn Conn, const char *db);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_CONN_H__ */
