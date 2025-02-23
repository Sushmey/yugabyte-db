/*-------------------------------------------------------------------------
 *
 * dependency.h
 *	  Routines to support inter-object dependencies.
 *
 *
 * Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/dependency.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "catalog/objectaddress.h"


/*
 * Precise semantics of a dependency relationship are specified by the
 * DependencyType code (which is stored in a "char" field in pg_depend,
 * so we assign ASCII-code values to the enumeration members).
 *
 * In all cases, a dependency relationship indicates that the referenced
 * object may not be dropped without also dropping the dependent object.
 * However, there are several subflavors:
 *
 * DEPENDENCY_NORMAL ('n'): normal relationship between separately-created
 * objects.  The dependent object may be dropped without affecting the
 * referenced object.  The referenced object may only be dropped by
 * specifying CASCADE, in which case the dependent object is dropped too.
 * Example: a table column has a normal dependency on its datatype.
 *
 * DEPENDENCY_AUTO ('a'): the dependent object can be dropped separately
 * from the referenced object, and should be automatically dropped
 * (regardless of RESTRICT or CASCADE mode) if the referenced object
 * is dropped.
 * Example: a named constraint on a table is made auto-dependent on
 * the table, so that it will go away if the table is dropped.
 *
 * DEPENDENCY_INTERNAL ('i'): the dependent object was created as part
 * of creation of the referenced object, and is really just a part of
 * its internal implementation.  A DROP of the dependent object will be
 * disallowed outright (we'll tell the user to issue a DROP against the
 * referenced object, instead).  A DROP of the referenced object will be
 * propagated through to drop the dependent object whether CASCADE is
 * specified or not.
 * Example: a trigger that's created to enforce a foreign-key constraint
 * is made internally dependent on the constraint's pg_constraint entry.
 *
 * DEPENDENCY_INTERNAL_AUTO ('I'): the dependent object was created as
 * part of creation of the referenced object, and is really just a part
 * of its internal implementation.  A DROP of the dependent object will
 * be disallowed outright (we'll tell the user to issue a DROP against the
 * referenced object, instead).  While a regular internal dependency will
 * prevent the dependent object from being dropped while any such
 * dependencies remain, DEPENDENCY_INTERNAL_AUTO will allow such a drop as
 * long as the object can be found by following any of such dependencies.
 * Example: an index on a partition is made internal-auto-dependent on
 * both the partition itself as well as on the index on the parent
 * partitioned table; so the partition index is dropped together with
 * either the partition it indexes, or with the parent index it is attached
 * to.

 * DEPENDENCY_EXTENSION ('e'): the dependent object is a member of the
 * extension that is the referenced object.  The dependent object can be
 * dropped only via DROP EXTENSION on the referenced object.  Functionally
 * this dependency type acts the same as an internal dependency, but it's
 * kept separate for clarity and to simplify pg_dump.
 *
 * DEPENDENCY_AUTO_EXTENSION ('x'): the dependent object is not a member
 * of the extension that is the referenced object (and so should not be
 * ignored by pg_dump), but cannot function without the extension and
 * should be dropped when the extension itself is.  The dependent object
 * may be dropped on its own as well.
 *
 * DEPENDENCY_PIN ('p'): there is no dependent object; this type of entry
 * is a signal that the system itself depends on the referenced object,
 * and so that object must never be deleted.  Entries of this type are
 * created only during initdb.  The fields for the dependent object
 * contain zeroes.
 *
 * Other dependency flavors may be needed in future.
 */

typedef enum DependencyType
{
	DEPENDENCY_NORMAL = 'n',
	DEPENDENCY_AUTO = 'a',
	DEPENDENCY_INTERNAL = 'i',
	DEPENDENCY_INTERNAL_AUTO = 'I',
	DEPENDENCY_EXTENSION = 'e',
	DEPENDENCY_AUTO_EXTENSION = 'x',
	DEPENDENCY_PIN = 'p'
} DependencyType;

/*
 * There is also a SharedDependencyType enum type that determines the exact
 * semantics of an entry in pg_shdepend.  Just like regular dependency entries,
 * any pg_shdepend entry means that the referenced object cannot be dropped
 * unless the dependent object is dropped at the same time.  There are some
 * additional rules however:
 *
 * (a) For a SHARED_DEPENDENCY_PIN entry, there is no dependent object --
 * rather, the referenced object is an essential part of the system.  This
 * applies to the initdb-created superuser.  Entries of this type are only
 * created by initdb; objects in this category don't need further pg_shdepend
 * entries if more objects come to depend on them.
 *
 * (b) a SHARED_DEPENDENCY_OWNER entry means that the referenced object is
 * the role owning the dependent object.  The referenced object must be
 * a pg_authid entry.
 *
 * (c) a SHARED_DEPENDENCY_ACL entry means that the referenced object is
 * a role mentioned in the ACL field of the dependent object.  The referenced
 * object must be a pg_authid entry.  (SHARED_DEPENDENCY_ACL entries are not
 * created for the owner of an object; hence two objects may be linked by
 * one or the other, but not both, of these dependency types.)
 *
 * (d) a SHARED_DEPENDENCY_POLICY entry means that the referenced object is
 * a role mentioned in a policy object.  The referenced object must be a
 * pg_authid entry.
 *
 * (e) a SHARED_DEPENDENCY_TABLESPACE entry means that the referenced
 * object is a tablespace mentioned in a relation without Postgres storage
 * (Yugabyte relations and relations with no files).  The referenced object
 * must be a pg_tablespace entry.  (Relations that have storage don't need
 * this: they are protected by the existence of a physical file in the
 * tablespace.)
 *
 * SHARED_DEPENDENCY_INVALID is a value used as a parameter in internal
 * routines, and is not valid in the catalog itself.
 */
typedef enum SharedDependencyType
{
	SHARED_DEPENDENCY_PIN = 'p',
	SHARED_DEPENDENCY_OWNER = 'o',
	SHARED_DEPENDENCY_ACL = 'a',
	SHARED_DEPENDENCY_POLICY = 'r',
	SHARED_DEPENDENCY_TABLESPACE = 't',
	SHARED_DEPENDENCY_INVALID = 0
} SharedDependencyType;

/* expansible list of ObjectAddresses (private in dependency.c) */
typedef struct ObjectAddresses ObjectAddresses;

/*
 * This enum covers all system catalogs whose OIDs can appear in
 * pg_depend.classId or pg_shdepend.classId.  Keep object_classes[] in sync.
 */
typedef enum ObjectClass
{
	OCLASS_CLASS,				/* pg_class */
	OCLASS_PROC,				/* pg_proc */
	OCLASS_TYPE,				/* pg_type */
	OCLASS_CAST,				/* pg_cast */
	OCLASS_COLLATION,			/* pg_collation */
	OCLASS_CONSTRAINT,			/* pg_constraint */
	OCLASS_CONVERSION,			/* pg_conversion */
	OCLASS_DEFAULT,				/* pg_attrdef */
	OCLASS_LANGUAGE,			/* pg_language */
	OCLASS_LARGEOBJECT,			/* pg_largeobject */
	OCLASS_OPERATOR,			/* pg_operator */
	OCLASS_OPCLASS,				/* pg_opclass */
	OCLASS_OPFAMILY,			/* pg_opfamily */
	OCLASS_AM,					/* pg_am */
	OCLASS_AMOP,				/* pg_amop */
	OCLASS_AMPROC,				/* pg_amproc */
	OCLASS_REWRITE,				/* pg_rewrite */
	OCLASS_TRIGGER,				/* pg_trigger */
	OCLASS_SCHEMA,				/* pg_namespace */
	OCLASS_STATISTIC_EXT,		/* pg_statistic_ext */
	OCLASS_TSPARSER,			/* pg_ts_parser */
	OCLASS_TSDICT,				/* pg_ts_dict */
	OCLASS_TSTEMPLATE,			/* pg_ts_template */
	OCLASS_TSCONFIG,			/* pg_ts_config */
	OCLASS_ROLE,				/* pg_authid */
	OCLASS_DATABASE,			/* pg_database */
	OCLASS_TBLGROUP, 			/* pg_yb_tablegroup */
	OCLASS_TBLSPACE,			/* pg_tablespace */
	OCLASS_FDW,					/* pg_foreign_data_wrapper */
	OCLASS_FOREIGN_SERVER,		/* pg_foreign_server */
	OCLASS_USER_MAPPING,		/* pg_user_mapping */
	OCLASS_DEFACL,				/* pg_default_acl */
	OCLASS_EXTENSION,			/* pg_extension */
	OCLASS_EVENT_TRIGGER,		/* pg_event_trigger */
	OCLASS_POLICY,				/* pg_policy */
	OCLASS_PUBLICATION,			/* pg_publication */
	OCLASS_PUBLICATION_REL,		/* pg_publication_rel */
	OCLASS_SUBSCRIPTION,		/* pg_subscription */
	OCLASS_TRANSFORM			/* pg_transform */
} ObjectClass;

#define LAST_OCLASS		OCLASS_TRANSFORM

/* flag bits for performDeletion/performMultipleDeletions: */
#define PERFORM_DELETION_INTERNAL			0x0001	/* internal action */
#define PERFORM_DELETION_CONCURRENTLY		0x0002	/* concurrent drop */
#define PERFORM_DELETION_QUIETLY			0x0004	/* suppress notices */
#define PERFORM_DELETION_SKIP_ORIGINAL		0x0008	/* keep original obj */
#define PERFORM_DELETION_SKIP_EXTENSIONS	0x0010	/* keep extensions */


/* in dependency.c */

extern void performDeletion(const ObjectAddress *object,
				DropBehavior behavior, int flags);

extern void performMultipleDeletions(const ObjectAddresses *objects,
						 DropBehavior behavior, int flags);

extern void recordDependencyOnExpr(const ObjectAddress *depender,
					   Node *expr, List *rtable,
					   DependencyType behavior);

extern void recordDependencyOnSingleRelExpr(const ObjectAddress *depender,
								Node *expr, Oid relId,
								DependencyType behavior,
								DependencyType self_behavior,
								bool ignore_self);

extern ObjectClass getObjectClass(const ObjectAddress *object);

extern ObjectAddresses *new_object_addresses(void);

extern void add_exact_object_address(const ObjectAddress *object,
						 ObjectAddresses *addrs);

extern bool object_address_present(const ObjectAddress *object,
					   const ObjectAddresses *addrs);

extern void record_object_address_dependencies(const ObjectAddress *depender,
								   ObjectAddresses *referenced,
								   DependencyType behavior);

extern void free_object_addresses(ObjectAddresses *addrs);

/* in pg_depend.c */

extern void recordDependencyOn(const ObjectAddress *depender,
				   const ObjectAddress *referenced,
				   DependencyType behavior);

extern void recordMultipleDependencies(const ObjectAddress *depender,
						   const ObjectAddress *referenced,
						   int nreferenced,
						   DependencyType behavior);

extern void recordDependencyOnCurrentExtension(const ObjectAddress *object,
								   bool isReplace);

extern void YBRecordPinDependency(const ObjectAddress *referenced, bool shared_insert);

extern void checkMembershipInCurrentExtension(const ObjectAddress *object);

extern long deleteDependencyRecordsFor(Oid classId, Oid objectId,
						   bool skipExtensionDeps);

extern long deleteDependencyRecordsForClass(Oid classId, Oid objectId,
								Oid refclassId, char deptype);

extern long changeDependencyFor(Oid classId, Oid objectId,
					Oid refClassId, Oid oldRefObjectId,
					Oid newRefObjectId);

extern Oid	getExtensionOfObject(Oid classId, Oid objectId);

extern bool sequenceIsOwned(Oid seqId, char deptype, Oid *tableId, int32 *colId);
extern List *getOwnedSequences(Oid relid, AttrNumber attnum);
extern Oid	getOwnedSequence(Oid relid, AttrNumber attnum);

extern Oid	get_constraint_index(Oid constraintId);

extern Oid	get_index_constraint(Oid indexId);

/* in pg_shdepend.c */

extern void recordSharedDependencyOn(ObjectAddress *depender,
						 ObjectAddress *referenced,
						 SharedDependencyType deptype);

extern void deleteSharedDependencyRecordsFor(Oid classId, Oid objectId,
								 int32 objectSubId);

extern void recordDependencyOnOwner(Oid classId, Oid objectId, Oid owner);

extern void changeDependencyOnOwner(Oid classId, Oid objectId,
						Oid newOwnerId);

extern void recordDependencyOnTablespace(Oid classId, Oid objectId,
										 Oid tablespace);

extern void changeDependencyOnTablespace(Oid classId, Oid objectId,
										 Oid newTablespaceId);

extern void updateAclDependencies(Oid classId, Oid objectId, int32 objectSubId,
					  Oid ownerId,
					  int noldmembers, Oid *oldmembers,
					  int nnewmembers, Oid *newmembers);

extern void recordDependencyOnTablespace(Oid classId, Oid objectId,
						Oid tablespaceOid);

extern bool checkSharedDependencies(Oid classId, Oid objectId,
						char **detail_msg, char **detail_log_msg);

extern void shdepLockAndCheckObject(Oid classId, Oid objectId);

extern void copyTemplateDependencies(Oid templateDbId, Oid newDbId);

extern void dropDatabaseDependencies(Oid databaseId);

extern void shdepDropOwned(List *relids, DropBehavior behavior);

extern void shdepReassignOwned(List *relids, Oid newrole);

#endif							/* DEPENDENCY_H */
