#include <sys/time.h>
#include <errno.h>
#include <libpq-fe.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "doorbelldb.h"
#include "doorbelldb_postgres.h"

#ifdef SYSLOG
# include <syslog.h>
#endif

PGconn *conn = NULL;
const char *doorbell;

void select_doorbell(const char *value) {
	char *end = NULL;

	errno = EINVAL;
	cerror("Doorbell value cannot be empty", value[0] == '\0');

	errno = 0;
	strtol(value, &end, 10);
	cerror(value, errno != 0);

	errno = EINVAL;
	cerror(value, end[0] != '\0');

	doorbell = value;
}

static bool db_connect(void) {
	PGresult *res = NULL;

	if (conn == NULL) {
		conn = PQconnectdb("");

		if (conn == NULL) {
			return false;
		} else {
			res = PQprepare(conn, "press_exists", "SELECT NULL FROM dingdong WHERE doorbell = $1 AND start = to_timestamp($2)", 2, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = PQprepare(conn, "press_on", "INSERT INTO dingdong (doorbell, start) VALUES($1, to_timestamp($2))", 2, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = PQprepare(conn, "press_off", "UPDATE dingdong SET stop = to_timestamp($3) WHERE doorbell = $1 and start = to_timestamp($2)", 3, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = PQprepare(conn, "press_on_off", "INSERT INTO dingdong (doorbell, start, stop) VALUES($1, to_timestamp($2), to_timestamp($3))", 3, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = PQprepare(conn, "press_cancel", "DELETE FROM dingdong WHERE doorbell = $1 and start = to_timestamp($2)", 2, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = PQprepare(conn, "press_resume", "UPDATE dingdong SET stop = NULL WHERE doorbell = $1 and start = to_timestamp($2)", 2, NULL);
			if (PQresultStatus(res) != PGRES_COMMAND_OK) goto fail;
			PQclear(res);

			res = NULL;
		}
	}

	if (PQstatus(conn) != CONNECTION_OK)
		goto fail;

	return true;

fail:
	_printf("db_connect: %s", PQerrorMessage(conn));
	if (res != NULL)
		PQclear(res);
	PQfinish(conn);
	conn = NULL;
	return false;
}

static void db_disconnect(void) {
	if (conn != NULL) {
		PGresult *res;

		res = PQexec(conn, "DEALLOCATE PREPARE press_exists");
		PQclear(res);

		res = PQexec(conn, "DEALLOCATE PREPARE press_on");
		PQclear(res);

		res = PQexec(conn, "DEALLOCATE PREPARE press_off");
		PQclear(res);

		res = PQexec(conn, "DEALLOCATE PREPARE press_on_off");
		PQclear(res);

		res = PQexec(conn, "DEALLOCATE PREPARE press_cancel");
		PQclear(res);

		res = PQexec(conn, "DEALLOCATE PREPARE press_resume");
		PQclear(res);

		PQfinish(conn);
		conn = NULL;
	}
}

bool press_on(const struct timeval *on) {
	PGresult *res;
	char tmp[1][32];
	const char *param[2] = { doorbell, tmp[0] };
	bool done = false;

	if (!db_connect())
		return false;

	sprintf(tmp[0], "%lu.%06u", (unsigned long int)on->tv_sec, (unsigned int)on->tv_usec);

	res = PQexecPrepared(conn, "press_exists", 2, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		_printf("press_exists: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		done = (PQntuples(res) > 0);

		PQclear(res);
	}

	if (done)
		return true;

	res = PQexecPrepared(conn, "press_on", 2, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_on: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		PQclear(res);
		return true;
	}
}

bool press_off(const struct timeval *on, const struct timeval *off) {
	PGresult *res;
	char tmp[2][32];
	const char *param[3] = { doorbell, tmp[0], tmp[1] };

	if (!db_connect())
		return false;

	sprintf(tmp[0], "%lu.%06u", (unsigned long int)on->tv_sec, (unsigned int)on->tv_usec);
	sprintf(tmp[1], "%lu.%06u", (unsigned long int)off->tv_sec, (unsigned int)off->tv_usec);

	res = PQexecPrepared(conn, "press_off", 3, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_off: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		PQclear(res);
		return true;
	}
}

bool press_on_off(const struct timeval *on, const struct timeval *off) {
	PGresult *res;
	char tmp[2][32];
	const char *param[3] = { doorbell, tmp[0], tmp[1] };
	bool done = false;

	if (!db_connect())
		return false;

	sprintf(tmp[0], "%lu.%06u", (unsigned long int)on->tv_sec, (unsigned int)on->tv_usec);
	sprintf(tmp[1], "%lu.%06u", (unsigned long int)off->tv_sec, (unsigned int)off->tv_usec);

	res = PQexecPrepared(conn, "press_off", 3, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_off: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		done = strcmp("0", PQcmdTuples(res));

		PQclear(res);
	}

	if (done)
		return true;

	res = PQexecPrepared(conn, "press_on_off", 3, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_on_off: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		PQclear(res);
		return true;
	}
}

bool press_cancel(const struct timeval *on) {
	PGresult *res;
	char tmp[1][32];
	const char *param[3] = { doorbell, tmp[0] };

	if (!db_connect())
		return false;

	sprintf(tmp[0], "%lu.%06u", (unsigned long int)on->tv_sec, (unsigned int)on->tv_usec);

	res = PQexecPrepared(conn, "press_cancel", 2, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_cancel: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		PQclear(res);
		return true;
	}
}

bool press_resume(const struct timeval *on) {
	PGresult *res;
	char tmp[1][32];
	const char *param[3] = { doorbell, tmp[0] };

	if (!db_connect())
		return false;

	sprintf(tmp[0], "%lu.%06u", (unsigned long int)on->tv_sec, (unsigned int)on->tv_usec);

	res = PQexecPrepared(conn, "press_resume", 2, param, NULL, NULL, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		_printf("press_resume: %s", PQerrorMessage(conn));

		PQclear(res);
		db_disconnect();
		return false;
	} else {
		PQclear(res);
		return true;
	}
}
