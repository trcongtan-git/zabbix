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

package zbxlib

/*
#cgo CFLAGS: -I${SRCDIR}/../../../../include

#include "zbxcommon.h"
#include "log.h"

int zbx_log_level = LOG_LEVEL_WARNING;
ZBX_THREAD_LOCAL int *zbx_plog_level = &zbx_log_level;

int	zbx_agent_pid;

void handleZabbixLog(int level, const char *message);

void __zbx_zabbix_log(int level, const char *format, ...)
{
	// no need to allocate memory for message if level is set to log.None (-1)
	if (zbx_agent_pid == getpid() && -1 != level)
	{
		va_list	args;
		char *message = NULL;
		size_t size;

		va_start(args, format);
		size = vsnprintf(NULL, 0, format, args) + 2;
		va_end(args);
		message = (char *)zbx_malloc(NULL, size);
		va_start(args, format);
		vsnprintf(message, size, format, args);
		va_end(args);

		handleZabbixLog(level, message);
		zbx_free(message);
	}
}

void	zbx_handle_log(void)
{
	// rotation is handled by go logger backend
}

int	zbx_redirect_stdio(const char *filename)
{
	// rotation is handled by go logger backend
	return FAIL;
}

*/
import "C"

import (
	"git.zabbix.com/ap/plugin-support/log"
)

func SetLogLevel(level int) {
	C.zbx_log_level = C.int(level)
}

func init() {
	log.Tracef("Calling C function \"getpid()\"")
	C.zbx_agent_pid = C.getpid()
}
