
PWD := $(shell pwd)

INSTALL = install
INSTALL_PROGRAM = $(INSTALL) --mode=755
INSTALL_DATA = $(INSTALL) --mode=644
INSTALL_DIR = $(INSTALL) -d --mode=755

CCFLAGS += -g -O1 -Wall -rdynamic -fPIC
CFLAGS += -g -Wall -rdynamic -fPIC

MAJOR_VERSION=1
MINOR_VERSION=0
REVISION=42284

LINKNAME=libnethosts.so
SONAME=$(LINKNAME).$(MAJOR_VERSION)
REALNAME=$(SONAME).$(MINOR_VERSION).$(REVISION)

default: rpm

rpm: dist

dist: build

build: $(LINKNAME) hosts libnss_sqlite.so


$(LINKNAME): $(SONAME)
	rm -f $(LINKNAME)
	ln -s $(SONAME) $(LINKNAME)

CLEANS += libhosts.o
$(SONAME): libhosts.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^ -lc

OBJS = hosts_tool.o

CLEANS += hosts $(OBJS)
hosts: $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $^ -L. -lnethosts -lsqlite3

CLEANS += nss_sqlite.o
CLEANS += libnss_sqlite.so
libnss_sqlite.so: nss_sqlite.o
	$(CC) -shared $^ -o $@ -lsqlite3

CLEANS += hosts.db
test:
	rm -f hosts.db
	sqlite3 hosts.db < hosts.sql
	LD_LIBRARY_PATH=. HOSTSDB=hosts.db ./hosts --debug --add fooname fe80::dead:beef --zone eth0
	LD_LIBRARY_PATH=. HOSTSDB=hosts.db ./hosts --debug --delete fooname fe80::dead:beef --zone eth0

install:
	# Add hosts library
	$(INSTALL) -d --mode=755 exports/usr/lib64
	$(INSTALL) --mode=755 $(SONAME)  exports/usr/lib64/$(REALNAME)
	rm -f exports/usr/lib64/$(SONAME)
	ln -s $(REALNAME) exports/usr/lib64/$(SONAME)
	rm -f exports/usr/lib64/$(LINKNAME)
	ln -s $(REALNAME) exports/usr/lib64/$(LINKNAME)
	# Add sqlite NSS plugin
	$(INSTALL) --mode=755 libnss_sqlite.so exports/usr/lib64/libnss_sqlite.so.2
	rm -f exports/usr/lib64/libnss_sqlite.so
	ln -s libnss_sqlite.so.2 exports/usr/lib64/libnss_sqlite.so
	# Add hosts library
	$(INSTALL) -d --mode=755 exports/usr/share/avance-network/
	$(INSTALL) --mode=755 hosts.sql exports/usr/share/avance-network/
	# Add hosts tool
	$(INSTALL) -d --mode=755 exports/usr/bin
	$(INSTALL) --mode=755 hosts exports/usr/bin

clean:
	$(RM) $(CLEANS)
	$(RM) -rf rpm exports

distclean: uninstall clean

.PHONY: test
