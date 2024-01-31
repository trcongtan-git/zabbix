/*
** Zabbix
** Copyright (C) 2001-2024 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#ifndef ZABBIX_COMMON_TRIM_UTF8_H
#define ZABBIX_COMMON_TRIM_UTF8_H

#define ZABBIX_MOCK_LTRIM_UTF8	0
#define ZABBIX_MOCK_RTRIM_UTF8	1

void	zbx_mock_test_entry_common_trim_utf8(void **state, int trim_utf8_func);
#endif
