#ifndef SYS_TYPES_H
	#include <sys/types.h>
#endif

#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#define _WANT_FREEBSD11_STAT 1
#define _WANT_FREEBSD11_KINFO 1
#define _WANT_FREEBSD11_PROC 1


#include "../include/sysctl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/param.h>

// OID descriptor
typedef struct sysctl_oid
{
	int	oid_number;
	int	oid_kind;
	int 	oid_format;
	const char *oid_name;
	const char *oid_desc;
	struct sysctl_oid *oid_parent;
	struct sysctl_oid *oid_children;
	struct sysctl_oid *oid_next;
} sysctl_oid_t;

//error handling
static const char *sysctl_error_string(sysctl_error_t err)
{
	switch(err)
	{
		case SYSCTL_OK:			return "SUCCESS";
		case SYSCTL_ERR_NO_MEMORY:	return "OUT OF MEMORY";
		case SYSCTL_ERR_PERMISSION:	return "PERMISSION DENIED";
		case SYSCTL_ERR_NOT_FOUND:	return "NOT FOUND";
		case SYSCTL_ERR_BUF_TOO_SMALLL:	return "BUFFER TOO SMALL";
		case SYSCTL_ERR_INVALID:	return "INVALID ARGUMENT";
		case SYSCTL_ERR_IO:		return "I/O ERROR";
		case SYSCTL_ERR_NOT_IMPL:	return "NOT IMPLEMENTED";
		case SYSCTL_ERR_BAD_TYPE:	return "BAD TYPE";
		case SYSCTL_ERR_CACHE_MISS:	return "CACHE MISS";
		default:			return "UNKNOWN ERROR";
	}
}

// MIB to NAME
static sysctl_error_t sysctl_mib_to_name(const int *mib, int mib_len,
		char *buffer, size_t *size)
{
	if (!mib || !buffer || !size) { return SYSCTL_ERR_INVALID; }

	if (sysctl(mib, mib_len, buffer, size, NULL, 0) < 0)
	{
		if (errno == ENOENT) return SYSCTL_ERR_NOT_FOUND;
		if (errno == EACCES) return SYSCTL_ERR_PERMISSION;
		return SYSCTL_ERR_IO;
	}

	return SYSCTL_OK;
}


// NAME to MIB
static sysctl_error_t sysctl_name_to_mib(const char *name, int *mib, int *mib_len)
{
	size_t len = CTL_MAXNAME;

	if (!name || !mib || !mib_len)
	{
		return SYSCTL_ERR_INVALID;
	}

	if (sysctlnametomib(name, mib, &len) < 0)
	{
		if (errno == ENOENT) return SYSCTL_ERR_NO_FOUND;
		if (errno == EACCES) return SYSCTL_ERR_PERMISSION;
		return SYSCTL_ERR_INVALID;
	}

	*mib_len = (int)len;
	return SYSCTL_OK;
}

//Detect OID type and format
static sysctl_error_t sysctl_detect_oid(const int *mib, int mib_len,
		sysctl_type_t *type, int *kind, char *format, size_t format_size)
{
	int type_mib[CTL_MAXNAME + 2];
	int val = 0;
	size_t len = sizeof(int);
	char fmt[64];
	size_t fmt_len = sizeof(fmt);
	int ret;

	if (!mib || !type)
	{
		return SYSCTL_ERR_INVALID;
	}

	// get OID kind flags
	memcpy(type_mib, mib, mib_len * sizeof(int));
	type_mib[mib_len] = CTLTYPE;

	if (sysctl(type_mib, mib_len + 1, &val, &len, NULL, 0) < 0)
	{
		*type = SYSCTL_TYPE_UNKNOWN;
		if (kind) *kind=0;
		return SYSCTL_ERR_IO;
	}

	if (kind) *kind = val;

	// determine type from CTLTYPE mask
	switch (val & CTLTYPE)
	{
		case CTLTYPE_INT:
			*type = SYSCTL_TYPE_INT;
			break;
		case CTLTYPE_LONG:
			*type = SYSCTL_TYPE_LONG;
			break;
		case CTLTYPE_UINT:
			*type = SYSCTL_TYPE_UINT;
			break;
		case CTLTYPE_STRING:
			*type = SYSCTL_TYPE_STRING;
			break;
		case CTLTYPE_ULONG:
			*type = SYSCTL_TYPE_ULONG;
			break;
		case CTLTYPE_OPAQUE:
			*type = SYSCTL_TYPE_OPAQUE;
			break;
		case CTLTYPE_NODE:
			*type = SYSCTL_TYPE_NODE;
			break;
		default:
			*type = SYSCTL_TYPE_UNKNOWN;
	}

	// get the format string
	if (format)
	{
		memcpy(type_mib, mib, mib_len * sizeof(int));
		type_mib[mib_len] = CTLFORMAT;
		fmt_len = sizeof(fmt);

		ret = sysctl(type_mib, mib_len + 1, fmt, &fmt_len, NULL, 0);
		if (ret == 0 && fmt_len > 0)
		{
			strncpy(format, fmt, format_size - 1);
			format[format_size - 1] = '\0';
		} else {
			format[0] = '\0';
		}
	}

	return SYSCTL_OK;
}

// read OID data
static sysctl_error_t sysctl_read_oid(const int *mib, int mib_len,
		void **data, size_t *data_len, sysctl_type_t *type)
{
	size_t len = 0;
	void *buf = NULL;
	sysctl_error_t err;
	sysctl_type_t val_type;
	int kind;
	char format[64];

	if (!mib || !data || !data_len)
	{
		return SYSCTL_ERR_INVALID;
	}

	*data = NULL;
	*data_len = 0;

	//detect type
	err = sysctl_detect_oid(mib, mib_len, &val_type, &kind, format, sizeof(format));
	if (err != SYSCTL_OK) { return err; }

	if (type) { *type = type_val; }

	if (val_type == SYSCTL_TYPE_NODE) { return SYSCTL_ERR_NOT_IMPL; }

	if (sysctl(mib, mib_len, NULL, &len, NULL, 0) < 0)
	{
		if (errno == EACCES) return SYSCTL_ERR_PERMISSION;
		return SYSCTL_ERR_IO;
	}

	if (len == 0)
	{
		*data = strdup("");
		if (!*data) return SYSCTL_ERR_NO_MEMORY;
		return SYSCTL_OK;
	}

	// allocate buffer
	buf = malloc(len + 1);
	if (!buf) { return SYSCTL_ERR_NO_MEMORY; }

	//read data
	if (sysctl(mib, mib_len, buf, &len, NULL, 0) < 0)
	{
		free(buf);
		if (errno == EACCES) return SYSCTL_ERR_PERMISSION;
		return SYSCTL_ERR_IO;
	}

	// null terminate strings
	if (val_type == SYSCTL_TYPE_STRING || 
			(val_type == SYSCTL_TYPE_OPAQUE && strcmp(format, "A") == 0))
	{
		char *str = (char *)buf;
		if (len > 0 && str[len-1] != '\0')
		{
			str[len] = '\0';
			len++;
		}
	}

	*data = buf;
	*data_len = len;

	return SYSCTL_OK;
}

