MODULES = translated

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
IDIRSV = $(shell $(PG_CONFIG) --includedir-server)
IDIR = $(shell $(PG_CONFIG) --includedir)
include $(PGXS)

translated.so: translated.o
	gcc -shared -o translated.so translated.o -lpq

translated.o: translated.c
	gcc -o translated.o -c translated.c $(CFLAGS) -I$(IDIRSV) -I$(IDIR)
