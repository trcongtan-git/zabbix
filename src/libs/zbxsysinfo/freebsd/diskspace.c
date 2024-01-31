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

#include "zbxsysinfo.h"
#include "../sysinfo.h"

#include "zbxjson.h"
#include "log.h"
#include "zbxalgo.h"
#include "inodes.h"

static zbx_mntopt_t mntopts[] = {
#if defined(MNTOPT_NAMES)
	MNTOPT_NAMES
#else
	{MNT_ASYNC,		"asynchronous"},
	{MNT_EXPORTED,		"NFS exported"},
	{MNT_LOCAL,		"local"},
	{MNT_NOATIME,		"noatime"},
#if defined(MNT_NODEV)
	{MNT_NODEV,		"nodev"},
#endif
	{MNT_NOEXEC,		"noexec"},
	{MNT_NOSUID,		"nosuid"},
	{MNT_NOSYMFOLLOW,	"nosymfollow"},
	{MNT_QUOTA,		"with quotas"},
	{MNT_RDONLY,		"read-only"},
	{MNT_SYNCHRONOUS,	"synchronous"},
	{MNT_UNION,		"union"},
	{MNT_NOCLUSTERR,	"noclusterr"},
	{MNT_NOCLUSTERW,	"noclusterw"},
	{MNT_SUIDDIR,		"suiddir"},
	{MNT_SOFTDEP,		"soft-updates"},
#if defined(MNT_SUJ)
	{MNT_SUJ,		"journaled soft-updates"},
#endif
	{MNT_MULTILABEL,	"multilabel"},
	{MNT_ACLS,		"acls"},
#if defined(MNT_NFS4ACLS)
	{MNT_NFS4ACLS,		"nfsv4acls"},
#endif
#if defined(MNT_GJOURNAL)
	{MNT_GJOURNAL,		"gjournal"},
#endif
#if defined(MNT_AUTOMOUNTED)
	{MNT_AUTOMOUNTED,	"automounted"},
#endif
#if defined(MNT_VERIFIED)
	{MNT_VERIFIED,		"verified"},
#endif
#if defined(MNT_UNTRUSTED)
	{MNT_UNTRUSTED,		"untrusted"},
#endif
	{0,			NULL}
#endif
};

static int	get_fs_size_stat(const char *fs, zbx_uint64_t *total, zbx_uint64_t *free,
		zbx_uint64_t *used, double *pfree, double *pused, char **error)
{
#ifdef HAVE_SYS_STATVFS_H
#	define ZBX_STATFS	statvfs
#	define ZBX_BSIZE	f_frsize
#else
#	define ZBX_STATFS	statfs
#	define ZBX_BSIZE	f_bsize
#endif
	struct ZBX_STATFS	s;

	if (0 != ZBX_STATFS(fs, &s))
	{
		*error = zbx_dsprintf(NULL, "Cannot obtain filesystem information: %s", zbx_strerror(errno));
		zabbix_log(LOG_LEVEL_DEBUG,"%s failed with error: %s",__func__, *error);
		return SYSINFO_RET_FAIL;
	}

	/* Available space could be negative (top bit set) if we hit disk space */
	/* reserved for non-privileged users. Treat it as 0.                    */
	if (0 != ZBX_IS_TOP_BIT_SET(s.f_bavail))
		s.f_bavail = 0;

	if (NULL != total)
		*total = (zbx_uint64_t)s.f_blocks * s.ZBX_BSIZE;

	if (NULL != free)
		*free = (zbx_uint64_t)s.f_bavail * s.ZBX_BSIZE;

	if (NULL != used)
		*used = (zbx_uint64_t)(s.f_blocks - s.f_bfree) * s.ZBX_BSIZE;

	if (NULL != pfree)
	{
		if (0 != s.f_blocks - s.f_bfree + s.f_bavail)
			*pfree = (double)(100.0 * s.f_bavail) / (s.f_blocks - s.f_bfree + s.f_bavail);
		else
			*pfree = 0;
	}

	if (NULL != pused)
	{
		if (0 != s.f_blocks - s.f_bfree + s.f_bavail)
			*pused = 100.0 - (double)(100.0 * s.f_bavail) / (s.f_blocks - s.f_bfree + s.f_bavail);
		else
			*pused = 0;
	}

	return SYSINFO_RET_OK;
#undef ZBX_STATFS
#undef ZBX_BSIZE
}

static int	vfs_fs_used(const char *fs, AGENT_RESULT *result)
{
	zbx_uint64_t	value;
	char		*error;

	if (SYSINFO_RET_OK != get_fs_size_stat(fs, NULL, NULL, &value, NULL, NULL, &error))
	{
		SET_MSG_RESULT(result, error);
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, value);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_free(const char *fs, AGENT_RESULT *result)
{
	zbx_uint64_t	value;
	char		*error;

	if (SYSINFO_RET_OK != get_fs_size_stat(fs, NULL, &value, NULL, NULL, NULL, &error))
	{
		SET_MSG_RESULT(result, error);
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, value);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_total(const char *fs, AGENT_RESULT *result)
{
	zbx_uint64_t	value;
	char		*error;

	if (SYSINFO_RET_OK != get_fs_size_stat(fs, &value, NULL, NULL, NULL, NULL, &error))
	{
		SET_MSG_RESULT(result, error);
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, value);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_pfree(const char *fs, AGENT_RESULT *result)
{
	double	value;
	char	*error;

	if (SYSINFO_RET_OK != get_fs_size_stat(fs, NULL, NULL, NULL, &value, NULL, &error))
	{
		SET_MSG_RESULT(result, error);
		return SYSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, value);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_pused(const char *fs, AGENT_RESULT *result)
{
	double	value;
	char	*error;

	if (SYSINFO_RET_OK != get_fs_size_stat(fs, NULL, NULL, NULL, NULL, &value, &error))
	{
		SET_MSG_RESULT(result, error);
		return SYSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, value);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_size_local(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	char	*fsname, *mode;

	if (2 < request->nparam)
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Too many parameters."));
		return SYSINFO_RET_FAIL;
	}

	fsname = get_rparam(request, 0);
	mode = get_rparam(request, 1);

	if (NULL == fsname || '\0' == *fsname)
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Invalid first parameter."));
		return SYSINFO_RET_FAIL;
	}

	if (NULL == mode || '\0' == *mode || 0 == strcmp(mode, "total"))	/* default parameter */
		return vfs_fs_total(fsname, result);
	if (0 == strcmp(mode, "free"))
		return vfs_fs_free(fsname, result);
	if (0 == strcmp(mode, "pfree"))
		return vfs_fs_pfree(fsname, result);
	if (0 == strcmp(mode, "used"))
		return vfs_fs_used(fsname, result);
	if (0 == strcmp(mode, "pused"))
		return vfs_fs_pused(fsname, result);

	SET_MSG_RESULT(result, zbx_strdup(NULL, "Invalid second parameter."));

	return SYSINFO_RET_FAIL;
}

int	vfs_fs_size(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_execute_threaded_metric(vfs_fs_size_local, request, result);
}

int	vfs_fs_discovery(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int		i, rc;
	struct statfs	*mntbuf;
	struct zbx_json	j;

	if (0 == (rc = getmntinfo(&mntbuf, MNT_NOWAIT)))
	{
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Cannot obtain system information: %s", zbx_strerror(errno)));
		return SYSINFO_RET_FAIL;
	}

	zbx_json_initarray(&j, ZBX_JSON_STAT_BUF_LEN);

	for (i = 0; i < rc; i++)
	{
		char	*options;

		options = zbx_format_mntopt_string(mntopts, mntbuf[i].f_flags & MNT_VISFLAGMASK);

		zbx_json_addobject(&j, NULL);
		zbx_json_addstring(&j, ZBX_LLD_MACRO_FSNAME, mntbuf[i].f_mntonname, ZBX_JSON_TYPE_STRING);
		zbx_json_addstring(&j, ZBX_LLD_MACRO_FSTYPE, mntbuf[i].f_fstypename, ZBX_JSON_TYPE_STRING);
		zbx_json_addstring(&j, ZBX_LLD_MACRO_FSOPTIONS, options, ZBX_JSON_TYPE_STRING);
		zbx_json_close(&j);

		zbx_free(options);
	}

	zbx_json_close(&j);

	SET_STR_RESULT(result, zbx_strdup(NULL, j.buffer));

	zbx_json_free(&j);

	return SYSINFO_RET_OK;
}

static int	vfs_fs_get_local(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int			i, rc;
	struct statfs		*mntbuf;
	struct zbx_json		j;
	zbx_uint64_t		total, not_used, used;
	zbx_uint64_t		itotal, inot_used, iused;
	double			pfree, pused;
	double			ipfree, ipused;
	char			*error;
	zbx_vector_ptr_t	mntpoints;
	zbx_mpoint_t		*mntpoint;
	zbx_fsname_t		fsname;
	int			ret = SYSINFO_RET_FAIL;

	if (0 == (rc = getmntinfo(&mntbuf, MNT_NOWAIT)))
	{
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Cannot obtain system information: %s", zbx_strerror(errno)));
		return SYSINFO_RET_FAIL;
	}

	zbx_vector_ptr_create(&mntpoints);

	for (i = 0; i < rc; i++)
	{
		fsname.mpoint = mntbuf[i].f_mntonname;

		if (SYSINFO_RET_OK != get_fs_size_stat(fsname.mpoint, &total, &not_used, &used, &pfree, &pused, &error))
		{
			zbx_free(error);
			continue;
		}
		if (SYSINFO_RET_OK != get_fs_inode_stat(fsname.mpoint, &itotal, &inot_used, &iused, &ipfree, &ipused,
				"pused", &error))
		{
			zbx_free(error);
			continue;
		}

		mntpoint = (zbx_mpoint_t *)zbx_malloc(NULL, sizeof(zbx_mpoint_t));
		zbx_strlcpy(mntpoint->fsname, fsname.mpoint, MAX_STRING_LEN);
		zbx_strlcpy(mntpoint->fstype, mntbuf[i].f_fstypename, MAX_STRING_LEN);
		mntpoint->bytes.total = total;
		mntpoint->bytes.used = used;
		mntpoint->bytes.not_used = not_used;
		mntpoint->bytes.pfree = pfree;
		mntpoint->bytes.pused = pused;
		mntpoint->inodes.total = itotal;
		mntpoint->inodes.used = iused;
		mntpoint->inodes.not_used = inot_used;
		mntpoint->inodes.pfree = ipfree;
		mntpoint->inodes.pused = ipused;
		mntpoint->options = zbx_format_mntopt_string(mntopts, mntbuf[i].f_flags & MNT_VISFLAGMASK);

		zbx_vector_ptr_append(&mntpoints, mntpoint);
	}

	if (0 == (rc = getmntinfo(&mntbuf, MNT_NOWAIT)))
	{
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Cannot obtain system information: %s", zbx_strerror(errno)));
		goto out;
	}

	zbx_json_initarray(&j, ZBX_JSON_STAT_BUF_LEN);

	for (i = 0; i < rc; i++)
	{
		int	idx;

		fsname.mpoint = mntbuf[i].f_mntonname;
		fsname.type = mntbuf[i].f_fstypename;

		if (FAIL != (idx = zbx_vector_ptr_search(&mntpoints, &fsname, zbx_fsname_compare)))
		{
			mntpoint = (zbx_mpoint_t *)mntpoints.values[idx];
			zbx_json_addobject(&j, NULL);
			zbx_json_addstring(&j, ZBX_SYSINFO_TAG_FSNAME, mntpoint->fsname, ZBX_JSON_TYPE_STRING);
			zbx_json_addstring(&j, ZBX_SYSINFO_TAG_FSTYPE, mntpoint->fstype, ZBX_JSON_TYPE_STRING);
			zbx_json_addobject(&j, ZBX_SYSINFO_TAG_BYTES);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_TOTAL, mntpoint->bytes.total);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_FREE, mntpoint->bytes.not_used);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_USED, mntpoint->bytes.used);
			zbx_json_addfloat(&j, ZBX_SYSINFO_TAG_PFREE, mntpoint->bytes.pfree);
			zbx_json_addfloat(&j, ZBX_SYSINFO_TAG_PUSED, mntpoint->bytes.pused);
			zbx_json_close(&j);
			zbx_json_addobject(&j, ZBX_SYSINFO_TAG_INODES);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_TOTAL, mntpoint->inodes.total);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_FREE, mntpoint->inodes.not_used);
			zbx_json_adduint64(&j, ZBX_SYSINFO_TAG_USED, mntpoint->inodes.used);
			zbx_json_addfloat(&j, ZBX_SYSINFO_TAG_PFREE, mntpoint->inodes.pfree);
			zbx_json_addfloat(&j, ZBX_SYSINFO_TAG_PUSED, mntpoint->inodes.pused);
			zbx_json_close(&j);
			zbx_json_addstring(&j, ZBX_SYSINFO_TAG_FSOPTIONS, mntpoint->options, ZBX_JSON_TYPE_STRING);
			zbx_json_close(&j);
		}
	}

	zbx_json_close(&j);

	SET_STR_RESULT(result, zbx_strdup(NULL, j.buffer));

	zbx_json_free(&j);
	ret = SYSINFO_RET_OK;
out:
	zbx_vector_ptr_clear_ext(&mntpoints, (zbx_clean_func_t)zbx_mpoints_free);
	zbx_vector_ptr_destroy(&mntpoints);

	return ret;
}

int	vfs_fs_get(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_execute_threaded_metric(vfs_fs_get_local, request, result);
}