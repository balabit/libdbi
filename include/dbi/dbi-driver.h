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

#ifndef __DBI_DRIVER_H__
#define __DBI_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dbi-type.h"

dbi_driver dbi_driver_list(dbi_driver Current); /* returns next driver. if current is NULL, return first driver. */
dbi_driver dbi_driver_open(const char *name); /* goes thru linked list until it finds the right one */
int dbi_driver_is_reserved_word(dbi_driver Driver, const char *word);
void *dbi_driver_specific_function(dbi_driver Driver, const char *name);
int dbi_driver_quote_string(dbi_driver Driver, char **orig);

const char *dbi_driver_get_name(dbi_driver Driver);
const char *dbi_driver_get_filename(dbi_driver Driver);
const char *dbi_driver_get_description(dbi_driver Driver);
const char *dbi_driver_get_maintainer(dbi_driver Driver);
const char *dbi_driver_get_url(dbi_driver Driver);
const char *dbi_driver_get_version(dbi_driver Driver);
const char *dbi_driver_get_date_compiled(dbi_driver Driver);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __DBI_DRIVER_H__ */
