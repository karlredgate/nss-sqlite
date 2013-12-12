--
-- SQLite Hosts database
--

CREATE TABLE host(      id INTEGER PRIMARY KEY,
                   address STRING COLLATE NOCASE,
                  hostname STRING COLLATE NOCASE,
                      zone STRING COLLATE NOCASE,
                     ctime DATE,
                     mtime DATE,
              CONSTRAINT pairUnique UNIQUE (address,hostname)
               );

CREATE INDEX by_address ON host(address);
CREATE INDEX by_name ON host(hostname);

CREATE TRIGGER create_host AFTER INSERT ON host
BEGIN
    UPDATE host SET ctime = DATETIME('NOW') WHERE rowid = new.rowid;
END;

CREATE TRIGGER touch_host AFTER UPDATE ON host
BEGIN
    UPDATE host SET mtime = DATETIME('NOW') WHERE rowid = new.rowid;
END;

insert into host (address, hostname) values ('127.0.0.1', 'localhost');
insert into host (address, hostname) values ('::1', 'localhost');
insert into host (address, hostname) values ('::1', 'ip6-localhost');
insert into host (address, hostname) values ('::1', 'ip6-loopback');
insert into host (address, hostname) values ('fe00::0', 'ip6-localnet');
insert into host (address, hostname) values ('ff00::0', 'ip6-mcastprefix');
insert into host (address, hostname) values ('ff02::1', 'ip6-allnodes');
insert into host (address, hostname) values ('ff02::2', 'ip6-allrouters');

-- foreign is a node that is not part of this cluster

CREATE TABLE node(  id INTEGER PRIMARY KEY,
                  uuid STRING COLLATE NOCASE,
                status STRING DEFAULT 'foreign',
                 ctime DATE,
                 mtime DATE,
                 CONSTRAINT uniqueUUID UNIQUE (uuid)
                  );

CREATE TRIGGER create_node AFTER INSERT ON node
BEGIN
    UPDATE node SET ctime = DATETIME('NOW') WHERE rowid = new.rowid;
END;

CREATE TRIGGER touch_node AFTER UPDATE ON node
BEGIN
    UPDATE node SET mtime = DATETIME('NOW') WHERE rowid = new.rowid;
END;

insert into node (uuid,status) values ('00000000-0000-0000-0000-000000000000','peer');

CREATE TABLE lan(  id INTEGER PRIMARY KEY,
                  uuid STRING COLLATE NOCASE,
                 ctime DATE,
                 mtime DATE,
                 CONSTRAINT uniqueUUID UNIQUE (uuid)
                  );
