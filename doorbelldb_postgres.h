#if 0
# ifdef VERBOSE
#define PQprepare(conn, name, sql, num, x) (\
	printf("PQprepare(%s) = %s\n", name, sql) \
, \
	PQprepare(conn, name, sql, num, x) \
)

#define PQexecPrepared(conn, name, num, param, x, y, z) (\
	({ \
		int __i; \
		printf("PQexec(%s)", name); \
		if (num > 0) { \
			printf(" { "); \
			for (__i = 0; __i < num; __i++) { \
				if (__i > 0) \
					printf(", "); \
				printf("%d = '%s'", __i, ((const char **)param)[__i]); \
			} \
			printf(" }"); \
		} \
		printf("\n"); \
	}) \
, \
	PQexecPrepared(conn, name, num, param, x, y, z) \
)
# endif
#endif
