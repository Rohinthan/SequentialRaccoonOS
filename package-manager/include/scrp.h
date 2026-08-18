#ifndef SCRP_H
#define SCRP_H

#define SCRP_VERSION "0.6.0"

#define SCRP_DATABASE "/var/lib/scrp/database"

int database_init(void);
int database_query(void);
int database_add(const char *name, const char *version);
int database_remove(const char *name);

#endif
