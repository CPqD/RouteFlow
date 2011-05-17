-------------------------------------------------------------------------------
-- Internal type-to-name mappings.
-------------------------------------------------------------------------------

CREATE TABLE NOX_PRINCIPAL_TYPE(
       ID               INTEGER PRIMARY KEY,
       NAME             TEXT,                  -- "Host", "Switch", "Address"...
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);


--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('Switch', 0, 0, 0, 0);
--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('Host', 0, 0, 0, 0);
--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('HostNetId', 0, 0, 0, 0);
--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('Address', 0, 0, 0, 0);
--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('Group', 0, 0, 0, 0);
--INSERT INTO NOX_PRINCIPAL_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('GroupMember', 0, 0, 0, 0);

CREATE TABLE NOX_ADDRESS_TYPE(
       ID               INTEGER PRIMARY KEY,
       NAME             TEXT,                  -- "MAC", "IPv4",
                                               -- "IPv6", "Alias".
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);
--INSERT INTO NOX_ADDRESS_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('MAC', 0, 0, 0, 0);
--INSERT INTO NOX_ADDRESS_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('IPv4', 0, 0, 0, 0);
--INSERT INTO NOX_ADDRESS_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('IPv6', 0, 0, 0, 0);
--INSERT INTO NOX_ADDRESS_TYPE(NAME, DELETED, CONTROLLER_ID, TRANSACTION_ID, MODIFIED_TS)
--       VALUES('Alias', 0, 0, 0, 0);

-------------------------------------------------------------------------------
-- Configured data sources.
-------------------------------------------------------------------------------

CREATE TABLE NOX_DATASOURCE_TYPE(
       ID               INTEGER PRIMARY KEY,
       NAME             TEXT,                  -- "Built-in", "LDAP", ...
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_DATASOURCE(
       ID               INTEGER PRIMARY KEY,   -- 'Config ID'
       TYPE_ID          INTEGER,               -- Refers to DATASOURCE_TYPE.ID
       NAME             TEXT,                  -- For example, "BigRadiusBox"
       SEARCH_ORDER     INTEGER,               -- Search order.

       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

-------------------------------------------------------------------------------
-- Principal tables.
-------------------------------------------------------------------------------

CREATE TABLE NOX_SWITCH(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       NAME             TEXT,                  -- User-friendly name.
       DESCRIPTION      TEXT,                  -- Free-formed description.
       DPID             INTEGER,               -- Switch assigned Datapath ID.

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_LOCATION(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       NAME             TEXT,                  -- User-friendly name.
       DESCRIPTION      TEXT,                  -- Free-formed description.
       SWITCH_ID        INTEGER,               -- Refers to SWITCH.ID
       PORT             INTEGER,               -- 
       PORT_NAME        TEXT,                  -- 
       SPEED            INTEGER,               --
       DUPLEX           INTEGER,               --
       AUTO_NEG         INTEGER,               --
       NEG_DOWN         INTEGER,               --
       ADMIN_STATE      INTEGER,               --

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_USER(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       NAME             TEXT,                  -- User-friendly name.
       REAL_NAME        TEXT,                  -- Full user name.
       DESCRIPTION      TEXT,                  -- Free-formed description.
       LOCATION         TEXT,                  -- Free-formed location.
       PHONE            TEXT,                  -- Free-formed phone #.
       USER_EMAIL       TEXT,                  -- Free-formed email address.

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_HOST(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       NAME             TEXT,                  -- User-friendly host name.
       DESCRIPTION      TEXT,                  -- Free-formed description.
       -- XXX: LOCAL ID (for authenticator to use?)

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_HOST_NETID(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       HOST_ID          INTEGER,               -- Refers to HOST.ID
       NAME             TEXT,                  -- User-friendly net info name.
       DESCRIPTION      TEXT,                  -- Free-formed description.
       IS_ROUTER        INTEGER,               -- Flag (0/1).
       IS_GATEWAY       INTEGER,               -- Flag (0/1).

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_ADDRESS(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       ADDRESS_TYPE     INTEGER,               -- Refers to ADDRESS_TYPE.ID
       ADDRESS          TEXT,                  -- Address in the type
                                               -- specified encoded
                                               -- format.
       HOST_NET_ID_ID   INTEGER,               -- HOST_NETID.ID, if
					       -- belongs to a net id.
       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

-------------------------------------------------------------------------------
-- Group tables are shared by all principal tables.
-------------------------------------------------------------------------------

CREATE TABLE NOX_GROUP(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       PRINCIPAL_TYPE   INTEGER,               -- Refers to PRINCIPAL_TYPE.ID.
                                               -- Couples the group to
                                               -- a principal, if any.
       PRINCIPAL_ID     INTEGER,               -- Refers to <MAIN_TABLE>.ID
                                               -- Couples the group to
                                               -- a principal, if any.
       MEMBER_TYPE      INTEGER,               -- If not NULL, all members
                                               -- are of this type.
       NAME             TEXT,                  -- User-friendly name.
       DESCRIPTION      TEXT,                  -- Free-formed description.

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID

       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_GROUP_MEMBER(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       GROUP_ID         INTEGER,               -- Refers to GROUP.ID
       PRINCIPAL_TYPE   INTEGER,               -- Refers to PRINCIPAL_TYPE.ID
       PRINCIPAL_ID     INTEGER,               -- Refers to <MAIN_TABLE>.ID

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER,               -- Modification time
                                               -- (ms since epoch).
       DEBUG            TEXT
);

-------------------------------------------------------------------------------
-- Credential tables are also shared by all principal tables.
-------------------------------------------------------------------------------

CREATE TABLE NOX_PRINCIPAL_PASSWORD(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       PRINCIPAL_TYPE   INTEGER,               -- Refers to TYPE.ID
       PRINCIPAL_ID     INTEGER,               -- Refers to <MAIN_TABLE>.ID
       SALT             TEXT,                  -- 
       HASH             TEXT,                  --
       UPDATE_TS        INTEGER,               -- Update time (epoch)
       EXPIRE_TS        INTEGER,               -- Expiration time (epoch)

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

CREATE TABLE NOX_PRINCIPAL_FP_CERT(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       PRINCIPAL_TYPE   INTEGER,               -- Refers to TYPE.ID
       PRINCIPAL_ID     INTEGER,               -- Refers to <MAIN_TABLE>.ID
       FINGERPRINT      TEXT,                  -- Certificate fingerprint.
       IS_APPROVED      INTEGER,               -- Flag (0/1).XXX:move elsewhere?

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

-------------------------------------------------------------------------------
-- Principals have may have additional, datasource specific
-- attributes not stored in their main tables.
-------------------------------------------------------------------------------

CREATE TABLE NOX_ATTRIBUTE_TYPE(
       ID                 INTEGER PRIMARY KEY,
       DATASOURCE_TYPE_ID INTEGER,               -- Refers to DATASOURCE_TYPE.ID
       NAME               TEXT,                  -- "Attr1", "Attr2",

       DELETED            INTEGER,               -- '0' if active, and
                                                 -- '1' if deleted.
       CONTROLLER_ID      INTEGER,               -- Controller ID.
       TRANSACTION_ID     INTEGER,               -- Controller's locally
                                                 -- assigned transaction ID.
       MODIFIED_TS        INTEGER                -- Modification time
                                                 -- (ms since epoch).
);

CREATE TABLE NOX_PRINCIPAL_ATTR(
       ID               INTEGER PRIMARY KEY,
       DATASOURCE_ID    INTEGER,               -- Refers to DATASOURCE.ID
       PRINCIPAL_TYPE   INTEGER,               -- Refers to TYPE.ID
       PRINCIPAL_ID     INTEGER,               -- Refers to <PRINCIPAL>.ID
       ATTRIBUTE_ID     INTEGER,               -- Unique attribute identifier.
       ATTRIBUTE_VALUE  TEXT                   -- 

       EXTERNAL_TYPE    INTEGER,               -- Refers to EXTERNAL_TYPE.ID
       DELETED          INTEGER,               -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID    INTEGER,               -- Controller ID.
       TRANSACTION_ID   INTEGER,               -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS      INTEGER                -- Modification time
                                               -- (ms since epoch).
);

-------------------------------------------------------------------------------
-- Datasources may record the datasource type specific principal type
-- information into the principal main tables.
-------------------------------------------------------------------------------
CREATE TABLE NOX_EXTERNAL_TYPE(
       ID                 INTEGER PRIMARY KEY,
       NAME               TEXT,                -- "Host", "Switch", "Address"...
       DATASOURCE_TYPE_ID INTEGER,             -- Refers to DATASOURCE_TYPE.ID
       DELETED            INTEGER,             -- '0' if active, and
                                               -- '1' if deleted.
       CONTROLLER_ID      INTEGER,             -- Controller ID.
       TRANSACTION_ID     INTEGER,             -- Controller's locally assigned
                                               -- transaction ID.
       MODIFIED_TS        INTEGER              -- Modification time
                                               -- (ms since epoch).
);