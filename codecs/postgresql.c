// thin wrapper around the pg_* postgresql stuff

#include "postgresql.h"
#include "pg_wchar.h"

int postgresql_is_valid_utf8 (size_t len, char *value)
{
	return pg_verify_mbstr_len(PG_UTF8, value, len, true, true) >= 0;
}

