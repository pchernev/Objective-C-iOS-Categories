
/************** Begin file loadext.c *****************************************/
/*
 ** 2006 June 7
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **
 *************************************************************************
 ** This file contains code used to dynamically load extensions into
 ** the SQLite library.
 */

#ifndef SQLITE_CORE
#define SQLITE_CORE 1  /* Disable the API redefinition in sqlite3ext.h */
#endif
/************** Include sqlite3ext.h in the middle of loadext.c **************/
/************** Begin file sqlite3ext.h **************************************/
/*
 ** 2006 June 7
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **
 *************************************************************************
 ** This header file defines the SQLite interface for use by
 ** shared libraries that want to be imported as extensions into
 ** an SQLite instance.  Shared libraries that intend to be loaded
 ** as extensions by SQLite should #include this file instead of
 ** sqlite3.h.
 */
#ifndef _SQLITE3EXT_H_
#define _SQLITE3EXT_H_

typedef struct sqlite3_api_routines sqlite3_api_routines;

/*
 ** The following structure holds pointers to all of the SQLite API
 ** routines.
 **
 ** WARNING:  In order to maintain backwards compatibility, add new
 ** interfaces to the end of this structure only.  If you insert new
 ** interfaces in the middle of this structure, then older different
 ** versions of SQLite will not be able to load each others' shared
 ** libraries!
 */
struct sqlite3_api_routines {
    void * (*aggregate_context)(sqlite3_context*,int nBytes);
    int  (*aggregate_count)(sqlite3_context*);
    int  (*bind_blob)(sqlite3_stmt*,int,const void*,int n,void(*)(void*));
    int  (*bind_double)(sqlite3_stmt*,int,double);
    int  (*bind_int)(sqlite3_stmt*,int,int);
    int  (*bind_int64)(sqlite3_stmt*,int,sqlite_int64);
    int  (*bind_null)(sqlite3_stmt*,int);
    int  (*bind_parameter_count)(sqlite3_stmt*);
    int  (*bind_parameter_index)(sqlite3_stmt*,const char*zName);
    const char * (*bind_parameter_name)(sqlite3_stmt*,int);
    int  (*bind_text)(sqlite3_stmt*,int,const char*,int n,void(*)(void*));
    int  (*bind_text16)(sqlite3_stmt*,int,const void*,int,void(*)(void*));
    int  (*bind_value)(sqlite3_stmt*,int,const sqlite3_value*);
    int  (*busy_handler)(sqlite3*,int(*)(void*,int),void*);
    int  (*busy_timeout)(sqlite3*,int ms);
    int  (*changes)(sqlite3*);
    int  (*close)(sqlite3*);
    int  (*collation_needed)(sqlite3*,void*,void(*)(void*,sqlite3*,
                                                    int eTextRep,const char*));
    int  (*collation_needed16)(sqlite3*,void*,void(*)(void*,sqlite3*,
                                                      int eTextRep,const void*));
    const void * (*column_blob)(sqlite3_stmt*,int iCol);
    int  (*column_bytes)(sqlite3_stmt*,int iCol);
    int  (*column_bytes16)(sqlite3_stmt*,int iCol);
    int  (*column_count)(sqlite3_stmt*pStmt);
    const char * (*column_database_name)(sqlite3_stmt*,int);
    const void * (*column_database_name16)(sqlite3_stmt*,int);
    const char * (*column_decltype)(sqlite3_stmt*,int i);
    const void * (*column_decltype16)(sqlite3_stmt*,int);
    double  (*column_double)(sqlite3_stmt*,int iCol);
    int  (*column_int)(sqlite3_stmt*,int iCol);
    sqlite_int64  (*column_int64)(sqlite3_stmt*,int iCol);
    const char * (*column_name)(sqlite3_stmt*,int);
    const void * (*column_name16)(sqlite3_stmt*,int);
    const char * (*column_origin_name)(sqlite3_stmt*,int);
    const void * (*column_origin_name16)(sqlite3_stmt*,int);
    const char * (*column_table_name)(sqlite3_stmt*,int);
    const void * (*column_table_name16)(sqlite3_stmt*,int);
    const unsigned char * (*column_text)(sqlite3_stmt*,int iCol);
    const void * (*column_text16)(sqlite3_stmt*,int iCol);
    int  (*column_type)(sqlite3_stmt*,int iCol);
    sqlite3_value* (*column_value)(sqlite3_stmt*,int iCol);
    void * (*commit_hook)(sqlite3*,int(*)(void*),void*);
    int  (*complete)(const char*sql);
    int  (*complete16)(const void*sql);
    int  (*create_collation)(sqlite3*,const char*,int,void*,
                             int(*)(void*,int,const void*,int,const void*));
    int  (*create_collation16)(sqlite3*,const void*,int,void*,
                               int(*)(void*,int,const void*,int,const void*));
    int  (*create_function)(sqlite3*,const char*,int,int,void*,
                            void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                            void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                            void (*xFinal)(sqlite3_context*));
    int  (*create_function16)(sqlite3*,const void*,int,int,void*,
                              void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                              void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                              void (*xFinal)(sqlite3_context*));
    int (*create_module)(sqlite3*,const char*,const sqlite3_module*,void*);
    int  (*data_count)(sqlite3_stmt*pStmt);
    sqlite3 * (*db_handle)(sqlite3_stmt*);
    int (*declare_vtab)(sqlite3*,const char*);
    int  (*enable_shared_cache)(int);
    int  (*errcode)(sqlite3*db);
    const char * (*errmsg)(sqlite3*);
    const void * (*errmsg16)(sqlite3*);
    int  (*exec)(sqlite3*,const char*,sqlite3_callback,void*,char**);
    int  (*expired)(sqlite3_stmt*);
    int  (*finalize)(sqlite3_stmt*pStmt);
    void  (*free)(void*);
    void  (*free_table)(char**result);
    int  (*get_autocommit)(sqlite3*);
    void * (*get_auxdata)(sqlite3_context*,int);
    int  (*get_table)(sqlite3*,const char*,char***,int*,int*,char**);
    int  (*global_recover)(void);
    void  (*interruptx)(sqlite3*);
    sqlite_int64  (*last_insert_rowid)(sqlite3*);
    const char * (*libversion)(void);
    int  (*libversion_number)(void);
    void *(*malloc)(int);
    char * (*mprintf)(const char*,...);
    int  (*open)(const char*,sqlite3**);
    int  (*open16)(const void*,sqlite3**);
    int  (*prepare)(sqlite3*,const char*,int,sqlite3_stmt**,const char**);
    int  (*prepare16)(sqlite3*,const void*,int,sqlite3_stmt**,const void**);
    void * (*profile)(sqlite3*,void(*)(void*,const char*,sqlite_uint64),void*);
    void  (*progress_handler)(sqlite3*,int,int(*)(void*),void*);
    void *(*realloc)(void*,int);
    int  (*reset)(sqlite3_stmt*pStmt);
    void  (*result_blob)(sqlite3_context*,const void*,int,void(*)(void*));
    void  (*result_double)(sqlite3_context*,double);
    void  (*result_error)(sqlite3_context*,const char*,int);
    void  (*result_error16)(sqlite3_context*,const void*,int);
    void  (*result_int)(sqlite3_context*,int);
    void  (*result_int64)(sqlite3_context*,sqlite_int64);
    void  (*result_null)(sqlite3_context*);
    void  (*result_text)(sqlite3_context*,const char*,int,void(*)(void*));
    void  (*result_text16)(sqlite3_context*,const void*,int,void(*)(void*));
    void  (*result_text16be)(sqlite3_context*,const void*,int,void(*)(void*));
    void  (*result_text16le)(sqlite3_context*,const void*,int,void(*)(void*));
    void  (*result_value)(sqlite3_context*,sqlite3_value*);
    void * (*rollback_hook)(sqlite3*,void(*)(void*),void*);
    int  (*set_authorizer)(sqlite3*,int(*)(void*,int,const char*,const char*,
                                           const char*,const char*),void*);
    void  (*set_auxdata)(sqlite3_context*,int,void*,void (*)(void*));
    char * (*snprintf)(int,char*,const char*,...);
    int  (*step)(sqlite3_stmt*);
    int  (*table_column_metadata)(sqlite3*,const char*,const char*,const char*,
                                  char const**,char const**,int*,int*,int*);
    void  (*thread_cleanup)(void);
    int  (*total_changes)(sqlite3*);
    void * (*trace)(sqlite3*,void(*xTrace)(void*,const char*),void*);
    int  (*transfer_bindings)(sqlite3_stmt*,sqlite3_stmt*);
    void * (*update_hook)(sqlite3*,void(*)(void*,int ,char const*,char const*,
                                           sqlite_int64),void*);
    void * (*user_data)(sqlite3_context*);
    const void * (*value_blob)(sqlite3_value*);
    int  (*value_bytes)(sqlite3_value*);
    int  (*value_bytes16)(sqlite3_value*);
    double  (*value_double)(sqlite3_value*);
    int  (*value_int)(sqlite3_value*);
    sqlite_int64  (*value_int64)(sqlite3_value*);
    int  (*value_numeric_type)(sqlite3_value*);
    const unsigned char * (*value_text)(sqlite3_value*);
    const void * (*value_text16)(sqlite3_value*);
    const void * (*value_text16be)(sqlite3_value*);
    const void * (*value_text16le)(sqlite3_value*);
    int  (*value_type)(sqlite3_value*);
    char *(*vmprintf)(const char*,va_list);
    /* Added ??? */
    int (*overload_function)(sqlite3*, const char *zFuncName, int nArg);
    /* Added by 3.3.13 */
    int (*prepare_v2)(sqlite3*,const char*,int,sqlite3_stmt**,const char**);
    int (*prepare16_v2)(sqlite3*,const void*,int,sqlite3_stmt**,const void**);
    int (*clear_bindings)(sqlite3_stmt*);
    /* Added by 3.4.1 */
    int (*create_module_v2)(sqlite3*,const char*,const sqlite3_module*,void*,
                            void (*xDestroy)(void *));
    /* Added by 3.5.0 */
    int (*bind_zeroblob)(sqlite3_stmt*,int,int);
    int (*blob_bytes)(sqlite3_blob*);
    int (*blob_close)(sqlite3_blob*);
    int (*blob_open)(sqlite3*,const char*,const char*,const char*,sqlite3_int64,
                     int,sqlite3_blob**);
    int (*blob_read)(sqlite3_blob*,void*,int,int);
    int (*blob_write)(sqlite3_blob*,const void*,int,int);
    int (*create_collation_v2)(sqlite3*,const char*,int,void*,
                               int(*)(void*,int,const void*,int,const void*),
                               void(*)(void*));
    int (*file_control)(sqlite3*,const char*,int,void*);
    sqlite3_int64 (*memory_highwater)(int);
    sqlite3_int64 (*memory_used)(void);
    sqlite3_mutex *(*mutex_alloc)(int);
    void (*mutex_enter)(sqlite3_mutex*);
    void (*mutex_free)(sqlite3_mutex*);
    void (*mutex_leave)(sqlite3_mutex*);
    int (*mutex_try)(sqlite3_mutex*);
    int (*open_v2)(const char*,sqlite3**,int,const char*);
    int (*release_memory)(int);
    void (*result_error_nomem)(sqlite3_context*);
    void (*result_error_toobig)(sqlite3_context*);
    int (*sleep)(int);
    void (*soft_heap_limit)(int);
    sqlite3_vfs *(*vfs_find)(const char*);
    int (*vfs_register)(sqlite3_vfs*,int);
    int (*vfs_unregister)(sqlite3_vfs*);
    int (*xthreadsafe)(void);
    void (*result_zeroblob)(sqlite3_context*,int);
    void (*result_error_code)(sqlite3_context*,int);
    int (*test_control)(int, ...);
    void (*randomness)(int,void*);
    sqlite3 *(*context_db_handle)(sqlite3_context*);
    int (*extended_result_codes)(sqlite3*,int);
    int (*limit)(sqlite3*,int,int);
    sqlite3_stmt *(*next_stmt)(sqlite3*,sqlite3_stmt*);
    const char *(*sql)(sqlite3_stmt*);
    int (*status)(int,int*,int*,int);
    int (*backup_finish)(sqlite3_backup*);
    sqlite3_backup *(*backup_init)(sqlite3*,const char*,sqlite3*,const char*);
    int (*backup_pagecount)(sqlite3_backup*);
    int (*backup_remaining)(sqlite3_backup*);
    int (*backup_step)(sqlite3_backup*,int);
    const char *(*compileoption_get)(int);
    int (*compileoption_used)(const char*);
    int (*create_function_v2)(sqlite3*,const char*,int,int,void*,
                              void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
                              void (*xStep)(sqlite3_context*,int,sqlite3_value**),
                              void (*xFinal)(sqlite3_context*),
                              void(*xDestroy)(void*));
    int (*db_config)(sqlite3*,int,...);
    sqlite3_mutex *(*db_mutex)(sqlite3*);
    int (*db_status)(sqlite3*,int,int*,int*,int);
    int (*extended_errcode)(sqlite3*);
    void (*log)(int,const char*,...);
    sqlite3_int64 (*soft_heap_limit64)(sqlite3_int64);
    const char *(*sourceid)(void);
    int (*stmt_status)(sqlite3_stmt*,int,int);
    int (*strnicmp)(const char*,const char*,int);
    int (*unlock_notify)(sqlite3*,void(*)(void**,int),void*);
    int (*wal_autocheckpoint)(sqlite3*,int);
    int (*wal_checkpoint)(sqlite3*,const char*);
    void *(*wal_hook)(sqlite3*,int(*)(void*,sqlite3*,const char*,int),void*);
    int (*blob_reopen)(sqlite3_blob*,sqlite3_int64);
    int (*vtab_config)(sqlite3*,int op,...);
    int (*vtab_on_conflict)(sqlite3*);
    /* Version 3.7.16 and later */
    int (*close_v2)(sqlite3*);
    const char *(*db_filename)(sqlite3*,const char*);
    int (*db_readonly)(sqlite3*,const char*);
    int (*db_release_memory)(sqlite3*);
    const char *(*errstr)(int);
    int (*stmt_busy)(sqlite3_stmt*);
    int (*stmt_readonly)(sqlite3_stmt*);
    int (*stricmp)(const char*,const char*);
    int (*uri_boolean)(const char*,const char*,int);
    sqlite3_int64 (*uri_int64)(const char*,const char*,sqlite3_int64);
    const char *(*uri_parameter)(const char*,const char*);
    char *(*vsnprintf)(int,char*,const char*,va_list);
    int (*wal_checkpoint_v2)(sqlite3*,const char*,int,int*,int*);
};

/*
 ** The following macros redefine the API routines so that they are
 ** redirected throught the global sqlite3_api structure.
 **
 ** This header file is also used by the loadext.c source file
 ** (part of the main SQLite library - not an extension) so that
 ** it can get access to the sqlite3_api_routines structure
 ** definition.  But the main library does not want to redefine
 ** the API.  So the redefinition macros are only valid if the
 ** SQLITE_CORE macros is undefined.
 */
#ifndef SQLITE_CORE
#define sqlite3_aggregate_context      sqlite3_api->aggregate_context
#ifndef SQLITE_OMIT_DEPRECATED
#define sqlite3_aggregate_count        sqlite3_api->aggregate_count
#endif
#define sqlite3_bind_blob              sqlite3_api->bind_blob
#define sqlite3_bind_double            sqlite3_api->bind_double
#define sqlite3_bind_int               sqlite3_api->bind_int
#define sqlite3_bind_int64             sqlite3_api->bind_int64
#define sqlite3_bind_null              sqlite3_api->bind_null
#define sqlite3_bind_parameter_count   sqlite3_api->bind_parameter_count
#define sqlite3_bind_parameter_index   sqlite3_api->bind_parameter_index
#define sqlite3_bind_parameter_name    sqlite3_api->bind_parameter_name
#define sqlite3_bind_text              sqlite3_api->bind_text
#define sqlite3_bind_text16            sqlite3_api->bind_text16
#define sqlite3_bind_value             sqlite3_api->bind_value
#define sqlite3_busy_handler           sqlite3_api->busy_handler
#define sqlite3_busy_timeout           sqlite3_api->busy_timeout
#define sqlite3_changes                sqlite3_api->changes
#define sqlite3_close                  sqlite3_api->close
#define sqlite3_collation_needed       sqlite3_api->collation_needed
#define sqlite3_collation_needed16     sqlite3_api->collation_needed16
#define sqlite3_column_blob            sqlite3_api->column_blob
#define sqlite3_column_bytes           sqlite3_api->column_bytes
#define sqlite3_column_bytes16         sqlite3_api->column_bytes16
#define sqlite3_column_count           sqlite3_api->column_count
#define sqlite3_column_database_name   sqlite3_api->column_database_name
#define sqlite3_column_database_name16 sqlite3_api->column_database_name16
#define sqlite3_column_decltype        sqlite3_api->column_decltype
#define sqlite3_column_decltype16      sqlite3_api->column_decltype16
#define sqlite3_column_double          sqlite3_api->column_double
#define sqlite3_column_int             sqlite3_api->column_int
#define sqlite3_column_int64           sqlite3_api->column_int64
#define sqlite3_column_name            sqlite3_api->column_name
#define sqlite3_column_name16          sqlite3_api->column_name16
#define sqlite3_column_origin_name     sqlite3_api->column_origin_name
#define sqlite3_column_origin_name16   sqlite3_api->column_origin_name16
#define sqlite3_column_table_name      sqlite3_api->column_table_name
#define sqlite3_column_table_name16    sqlite3_api->column_table_name16
#define sqlite3_column_text            sqlite3_api->column_text
#define sqlite3_column_text16          sqlite3_api->column_text16
#define sqlite3_column_type            sqlite3_api->column_type
#define sqlite3_column_value           sqlite3_api->column_value
#define sqlite3_commit_hook            sqlite3_api->commit_hook
#define sqlite3_complete               sqlite3_api->complete
#define sqlite3_complete16             sqlite3_api->complete16
#define sqlite3_create_collation       sqlite3_api->create_collation
#define sqlite3_create_collation16     sqlite3_api->create_collation16
#define sqlite3_create_function        sqlite3_api->create_function
#define sqlite3_create_function16      sqlite3_api->create_function16
#define sqlite3_create_module          sqlite3_api->create_module
#define sqlite3_create_module_v2       sqlite3_api->create_module_v2
#define sqlite3_data_count             sqlite3_api->data_count
#define sqlite3_db_handle              sqlite3_api->db_handle
#define sqlite3_declare_vtab           sqlite3_api->declare_vtab
#define sqlite3_enable_shared_cache    sqlite3_api->enable_shared_cache
#define sqlite3_errcode                sqlite3_api->errcode
#define sqlite3_errmsg                 sqlite3_api->errmsg
#define sqlite3_errmsg16               sqlite3_api->errmsg16
#define sqlite3_exec                   sqlite3_api->exec
#ifndef SQLITE_OMIT_DEPRECATED
#define sqlite3_expired                sqlite3_api->expired
#endif
#define sqlite3_finalize               sqlite3_api->finalize
#define sqlite3_free                   sqlite3_api->free
#define sqlite3_free_table             sqlite3_api->free_table
#define sqlite3_get_autocommit         sqlite3_api->get_autocommit
#define sqlite3_get_auxdata            sqlite3_api->get_auxdata
#define sqlite3_get_table              sqlite3_api->get_table
#ifndef SQLITE_OMIT_DEPRECATED
#define sqlite3_global_recover         sqlite3_api->global_recover
#endif
#define sqlite3_interrupt              sqlite3_api->interruptx
#define sqlite3_last_insert_rowid      sqlite3_api->last_insert_rowid
#define sqlite3_libversion             sqlite3_api->libversion
#define sqlite3_libversion_number      sqlite3_api->libversion_number
#define sqlite3_malloc                 sqlite3_api->malloc
#define sqlite3_mprintf                sqlite3_api->mprintf
#define sqlite3_open                   sqlite3_api->open
#define sqlite3_open16                 sqlite3_api->open16
#define sqlite3_prepare                sqlite3_api->prepare
#define sqlite3_prepare16              sqlite3_api->prepare16
#define sqlite3_prepare_v2             sqlite3_api->prepare_v2
#define sqlite3_prepare16_v2           sqlite3_api->prepare16_v2
#define sqlite3_profile                sqlite3_api->profile
#define sqlite3_progress_handler       sqlite3_api->progress_handler
#define sqlite3_realloc                sqlite3_api->realloc
#define sqlite3_reset                  sqlite3_api->reset
#define sqlite3_result_blob            sqlite3_api->result_blob
#define sqlite3_result_double          sqlite3_api->result_double
#define sqlite3_result_error           sqlite3_api->result_error
#define sqlite3_result_error16         sqlite3_api->result_error16
#define sqlite3_result_int             sqlite3_api->result_int
#define sqlite3_result_int64           sqlite3_api->result_int64
#define sqlite3_result_null            sqlite3_api->result_null
#define sqlite3_result_text            sqlite3_api->result_text
#define sqlite3_result_text16          sqlite3_api->result_text16
#define sqlite3_result_text16be        sqlite3_api->result_text16be
#define sqlite3_result_text16le        sqlite3_api->result_text16le
#define sqlite3_result_value           sqlite3_api->result_value
#define sqlite3_rollback_hook          sqlite3_api->rollback_hook
#define sqlite3_set_authorizer         sqlite3_api->set_authorizer
#define sqlite3_set_auxdata            sqlite3_api->set_auxdata
#define sqlite3_snprintf               sqlite3_api->snprintf
#define sqlite3_step                   sqlite3_api->step
#define sqlite3_table_column_metadata  sqlite3_api->table_column_metadata
#define sqlite3_thread_cleanup         sqlite3_api->thread_cleanup
#define sqlite3_total_changes          sqlite3_api->total_changes
#define sqlite3_trace                  sqlite3_api->trace
#ifndef SQLITE_OMIT_DEPRECATED
#define sqlite3_transfer_bindings      sqlite3_api->transfer_bindings
#endif
#define sqlite3_update_hook            sqlite3_api->update_hook
#define sqlite3_user_data              sqlite3_api->user_data
#define sqlite3_value_blob             sqlite3_api->value_blob
#define sqlite3_value_bytes            sqlite3_api->value_bytes
#define sqlite3_value_bytes16          sqlite3_api->value_bytes16
#define sqlite3_value_double           sqlite3_api->value_double
#define sqlite3_value_int              sqlite3_api->value_int
#define sqlite3_value_int64            sqlite3_api->value_int64
#define sqlite3_value_numeric_type     sqlite3_api->value_numeric_type
#define sqlite3_value_text             sqlite3_api->value_text
#define sqlite3_value_text16           sqlite3_api->value_text16
#define sqlite3_value_text16be         sqlite3_api->value_text16be
#define sqlite3_value_text16le         sqlite3_api->value_text16le
#define sqlite3_value_type             sqlite3_api->value_type
#define sqlite3_vmprintf               sqlite3_api->vmprintf
#define sqlite3_overload_function      sqlite3_api->overload_function
#define sqlite3_prepare_v2             sqlite3_api->prepare_v2
#define sqlite3_prepare16_v2           sqlite3_api->prepare16_v2
#define sqlite3_clear_bindings         sqlite3_api->clear_bindings
#define sqlite3_bind_zeroblob          sqlite3_api->bind_zeroblob
#define sqlite3_blob_bytes             sqlite3_api->blob_bytes
#define sqlite3_blob_close             sqlite3_api->blob_close
#define sqlite3_blob_open              sqlite3_api->blob_open
#define sqlite3_blob_read              sqlite3_api->blob_read
#define sqlite3_blob_write             sqlite3_api->blob_write
#define sqlite3_create_collation_v2    sqlite3_api->create_collation_v2
#define sqlite3_file_control           sqlite3_api->file_control
#define sqlite3_memory_highwater       sqlite3_api->memory_highwater
#define sqlite3_memory_used            sqlite3_api->memory_used
#define sqlite3_mutex_alloc            sqlite3_api->mutex_alloc
#define sqlite3_mutex_enter            sqlite3_api->mutex_enter
#define sqlite3_mutex_free             sqlite3_api->mutex_free
#define sqlite3_mutex_leave            sqlite3_api->mutex_leave
#define sqlite3_mutex_try              sqlite3_api->mutex_try
#define sqlite3_open_v2                sqlite3_api->open_v2
#define sqlite3_release_memory         sqlite3_api->release_memory
#define sqlite3_result_error_nomem     sqlite3_api->result_error_nomem
#define sqlite3_result_error_toobig    sqlite3_api->result_error_toobig
#define sqlite3_sleep                  sqlite3_api->sleep
#define sqlite3_soft_heap_limit        sqlite3_api->soft_heap_limit
#define sqlite3_vfs_find               sqlite3_api->vfs_find
#define sqlite3_vfs_register           sqlite3_api->vfs_register
#define sqlite3_vfs_unregister         sqlite3_api->vfs_unregister
#define sqlite3_threadsafe             sqlite3_api->xthreadsafe
#define sqlite3_result_zeroblob        sqlite3_api->result_zeroblob
#define sqlite3_result_error_code      sqlite3_api->result_error_code
#define sqlite3_test_control           sqlite3_api->test_control
#define sqlite3_randomness             sqlite3_api->randomness
#define sqlite3_context_db_handle      sqlite3_api->context_db_handle
#define sqlite3_extended_result_codes  sqlite3_api->extended_result_codes
#define sqlite3_limit                  sqlite3_api->limit
#define sqlite3_next_stmt              sqlite3_api->next_stmt
#define sqlite3_sql                    sqlite3_api->sql
#define sqlite3_status                 sqlite3_api->status
#define sqlite3_backup_finish          sqlite3_api->backup_finish
#define sqlite3_backup_init            sqlite3_api->backup_init
#define sqlite3_backup_pagecount       sqlite3_api->backup_pagecount
#define sqlite3_backup_remaining       sqlite3_api->backup_remaining
#define sqlite3_backup_step            sqlite3_api->backup_step
#define sqlite3_compileoption_get      sqlite3_api->compileoption_get
#define sqlite3_compileoption_used     sqlite3_api->compileoption_used
#define sqlite3_create_function_v2     sqlite3_api->create_function_v2
#define sqlite3_db_config              sqlite3_api->db_config
#define sqlite3_db_mutex               sqlite3_api->db_mutex
#define sqlite3_db_status              sqlite3_api->db_status
#define sqlite3_extended_errcode       sqlite3_api->extended_errcode
#define sqlite3_log                    sqlite3_api->log
#define sqlite3_soft_heap_limit64      sqlite3_api->soft_heap_limit64
#define sqlite3_sourceid               sqlite3_api->sourceid
#define sqlite3_stmt_status            sqlite3_api->stmt_status
#define sqlite3_strnicmp               sqlite3_api->strnicmp
#define sqlite3_unlock_notify          sqlite3_api->unlock_notify
#define sqlite3_wal_autocheckpoint     sqlite3_api->wal_autocheckpoint
#define sqlite3_wal_checkpoint         sqlite3_api->wal_checkpoint
#define sqlite3_wal_hook               sqlite3_api->wal_hook
#define sqlite3_blob_reopen            sqlite3_api->blob_reopen
#define sqlite3_vtab_config            sqlite3_api->vtab_config
#define sqlite3_vtab_on_conflict       sqlite3_api->vtab_on_conflict
/* Version 3.7.16 and later */
#define sqlite3_close_v2               sqlite3_api->close_v2
#define sqlite3_db_filename            sqlite3_api->db_filename
#define sqlite3_db_readonly            sqlite3_api->db_readonly
#define sqlite3_db_release_memory      sqlite3_api->db_release_memory
#define sqlite3_errstr                 sqlite3_api->errstr
#define sqlite3_stmt_busy              sqlite3_api->stmt_busy
#define sqlite3_stmt_readonly          sqlite3_api->stmt_readonly
#define sqlite3_stricmp                sqlite3_api->stricmp
#define sqlite3_uri_boolean            sqlite3_api->uri_boolean
#define sqlite3_uri_int64              sqlite3_api->uri_int64
#define sqlite3_uri_parameter          sqlite3_api->uri_parameter
#define sqlite3_uri_vsnprintf          sqlite3_api->vsnprintf
#define sqlite3_wal_checkpoint_v2      sqlite3_api->wal_checkpoint_v2
#endif /* SQLITE_CORE */

#ifndef SQLITE_CORE
/* This case when the file really is being compiled as a loadable
 ** extension */
# define SQLITE_EXTENSION_INIT1     const sqlite3_api_routines *sqlite3_api=0;
# define SQLITE_EXTENSION_INIT2(v)  sqlite3_api=v;
# define SQLITE_EXTENSION_INIT3     \
extern const sqlite3_api_routines *sqlite3_api;
#else
/* This case when the file is being statically linked into the
 ** application */
# define SQLITE_EXTENSION_INIT1     /*no-op*/
# define SQLITE_EXTENSION_INIT2(v)  (void)v; /* unused parameter */
# define SQLITE_EXTENSION_INIT3     /*no-op*/
#endif

#endif /* _SQLITE3EXT_H_ */

/************** End of sqlite3ext.h ******************************************/
/************** Continuing where we left off in loadext.c ********************/
/* #include <string.h> */

#ifndef SQLITE_OMIT_LOAD_EXTENSION

/*
 ** Some API routines are omitted when various features are
 ** excluded from a build of SQLite.  Substitute a NULL pointer
 ** for any missing APIs.
 */
#ifndef SQLITE_ENABLE_COLUMN_METADATA
# define sqlite3_column_database_name   0
# define sqlite3_column_database_name16 0
# define sqlite3_column_table_name      0
# define sqlite3_column_table_name16    0
# define sqlite3_column_origin_name     0
# define sqlite3_column_origin_name16   0
# define sqlite3_table_column_metadata  0
#endif

#ifdef SQLITE_OMIT_AUTHORIZATION
# define sqlite3_set_authorizer         0
#endif

#ifdef SQLITE_OMIT_UTF16
# define sqlite3_bind_text16            0
# define sqlite3_collation_needed16     0
# define sqlite3_column_decltype16      0
# define sqlite3_column_name16          0
# define sqlite3_column_text16          0
# define sqlite3_complete16             0
# define sqlite3_create_collation16     0
# define sqlite3_create_function16      0
# define sqlite3_errmsg16               0
# define sqlite3_open16                 0
# define sqlite3_prepare16              0
# define sqlite3_prepare16_v2           0
# define sqlite3_result_error16         0
# define sqlite3_result_text16          0
# define sqlite3_result_text16be        0
# define sqlite3_result_text16le        0
# define sqlite3_value_text16           0
# define sqlite3_value_text16be         0
# define sqlite3_value_text16le         0
# define sqlite3_column_database_name16 0
# define sqlite3_column_table_name16    0
# define sqlite3_column_origin_name16   0
#endif

#ifdef SQLITE_OMIT_COMPLETE
# define sqlite3_complete 0
# define sqlite3_complete16 0
#endif

#ifdef SQLITE_OMIT_DECLTYPE
# define sqlite3_column_decltype16      0
# define sqlite3_column_decltype        0
#endif

#ifdef SQLITE_OMIT_PROGRESS_CALLBACK
# define sqlite3_progress_handler 0
#endif

#ifdef SQLITE_OMIT_VIRTUALTABLE
# define sqlite3_create_module 0
# define sqlite3_create_module_v2 0
# define sqlite3_declare_vtab 0
# define sqlite3_vtab_config 0
# define sqlite3_vtab_on_conflict 0
#endif

#ifdef SQLITE_OMIT_SHARED_CACHE
# define sqlite3_enable_shared_cache 0
#endif

#ifdef SQLITE_OMIT_TRACE
# define sqlite3_profile       0
# define sqlite3_trace         0
#endif

#ifdef SQLITE_OMIT_GET_TABLE
# define sqlite3_free_table    0
# define sqlite3_get_table     0
#endif

#ifdef SQLITE_OMIT_INCRBLOB
#define sqlite3_bind_zeroblob  0
#define sqlite3_blob_bytes     0
#define sqlite3_blob_close     0
#define sqlite3_blob_open      0
#define sqlite3_blob_read      0
#define sqlite3_blob_write     0
#define sqlite3_blob_reopen    0
#endif

/*
 ** The following structure contains pointers to all SQLite API routines.
 ** A pointer to this structure is passed into extensions when they are
 ** loaded so that the extension can make calls back into the SQLite
 ** library.
 **
 ** When adding new APIs, add them to the bottom of this structure
 ** in order to preserve backwards compatibility.
 **
 ** Extensions that use newer APIs should first call the
 ** sqlite3_libversion_number() to make sure that the API they
 ** intend to use is supported by the library.  Extensions should
 ** also check to make sure that the pointer to the function is
 ** not NULL before calling it.
 */
static const sqlite3_api_routines sqlite3Apis = {
    sqlite3_aggregate_context,
#ifndef SQLITE_OMIT_DEPRECATED
    sqlite3_aggregate_count,
#else
    0,
#endif
    sqlite3_bind_blob,
    sqlite3_bind_double,
    sqlite3_bind_int,
    sqlite3_bind_int64,
    sqlite3_bind_null,
    sqlite3_bind_parameter_count,
    sqlite3_bind_parameter_index,
    sqlite3_bind_parameter_name,
    sqlite3_bind_text,
    sqlite3_bind_text16,
    sqlite3_bind_value,
    sqlite3_busy_handler,
    sqlite3_busy_timeout,
    sqlite3_changes,
    sqlite3_close,
    sqlite3_collation_needed,
    sqlite3_collation_needed16,
    sqlite3_column_blob,
    sqlite3_column_bytes,
    sqlite3_column_bytes16,
    sqlite3_column_count,
    sqlite3_column_database_name,
    sqlite3_column_database_name16,
    sqlite3_column_decltype,
    sqlite3_column_decltype16,
    sqlite3_column_double,
    sqlite3_column_int,
    sqlite3_column_int64,
    sqlite3_column_name,
    sqlite3_column_name16,
    sqlite3_column_origin_name,
    sqlite3_column_origin_name16,
    sqlite3_column_table_name,
    sqlite3_column_table_name16,
    sqlite3_column_text,
    sqlite3_column_text16,
    sqlite3_column_type,
    sqlite3_column_value,
    sqlite3_commit_hook,
    sqlite3_complete,
    sqlite3_complete16,
    sqlite3_create_collation,
    sqlite3_create_collation16,
    sqlite3_create_function,
    sqlite3_create_function16,
    sqlite3_create_module,
    sqlite3_data_count,
    sqlite3_db_handle,
    sqlite3_declare_vtab,
    sqlite3_enable_shared_cache,
    sqlite3_errcode,
    sqlite3_errmsg,
    sqlite3_errmsg16,
    sqlite3_exec,
#ifndef SQLITE_OMIT_DEPRECATED
    sqlite3_expired,
#else
    0,
#endif
    sqlite3_finalize,
    sqlite3_free,
    sqlite3_free_table,
    sqlite3_get_autocommit,
    sqlite3_get_auxdata,
    sqlite3_get_table,
    0,     /* Was sqlite3_global_recover(), but that function is deprecated */
    sqlite3_interrupt,
    sqlite3_last_insert_rowid,
    sqlite3_libversion,
    sqlite3_libversion_number,
    sqlite3_malloc,
    sqlite3_mprintf,
    sqlite3_open,
    sqlite3_open16,
    sqlite3_prepare,
    sqlite3_prepare16,
    sqlite3_profile,
    sqlite3_progress_handler,
    sqlite3_realloc,
    sqlite3_reset,
    sqlite3_result_blob,
    sqlite3_result_double,
    sqlite3_result_error,
    sqlite3_result_error16,
    sqlite3_result_int,
    sqlite3_result_int64,
    sqlite3_result_null,
    sqlite3_result_text,
    sqlite3_result_text16,
    sqlite3_result_text16be,
    sqlite3_result_text16le,
    sqlite3_result_value,
    sqlite3_rollback_hook,
    sqlite3_set_authorizer,
    sqlite3_set_auxdata,
    sqlite3_snprintf,
    sqlite3_step,
    sqlite3_table_column_metadata,
#ifndef SQLITE_OMIT_DEPRECATED
    sqlite3_thread_cleanup,
#else
    0,
#endif
    sqlite3_total_changes,
    sqlite3_trace,
#ifndef SQLITE_OMIT_DEPRECATED
    sqlite3_transfer_bindings,
#else
    0,
#endif
    sqlite3_update_hook,
    sqlite3_user_data,
    sqlite3_value_blob,
    sqlite3_value_bytes,
    sqlite3_value_bytes16,
    sqlite3_value_double,
    sqlite3_value_int,
    sqlite3_value_int64,
    sqlite3_value_numeric_type,
    sqlite3_value_text,
    sqlite3_value_text16,
    sqlite3_value_text16be,
    sqlite3_value_text16le,
    sqlite3_value_type,
    sqlite3_vmprintf,
    /*
     ** The original API set ends here.  All extensions can call any
     ** of the APIs above provided that the pointer is not NULL.  But
     ** before calling APIs that follow, extension should check the
     ** sqlite3_libversion_number() to make sure they are dealing with
     ** a library that is new enough to support that API.
     *************************************************************************
     */
    sqlite3_overload_function,
    
    /*
     ** Added after 3.3.13
     */
    sqlite3_prepare_v2,
    sqlite3_prepare16_v2,
    sqlite3_clear_bindings,
    
    /*
     ** Added for 3.4.1
     */
    sqlite3_create_module_v2,
    
    /*
     ** Added for 3.5.0
     */
    sqlite3_bind_zeroblob,
    sqlite3_blob_bytes,
    sqlite3_blob_close,
    sqlite3_blob_open,
    sqlite3_blob_read,
    sqlite3_blob_write,
    sqlite3_create_collation_v2,
    sqlite3_file_control,
    sqlite3_memory_highwater,
    sqlite3_memory_used,
#ifdef SQLITE_MUTEX_OMIT
    0,
    0,
    0,
    0,
    0,
#else
    sqlite3_mutex_alloc,
    sqlite3_mutex_enter,
    sqlite3_mutex_free,
    sqlite3_mutex_leave,
    sqlite3_mutex_try,
#endif
    sqlite3_open_v2,
    sqlite3_release_memory,
    sqlite3_result_error_nomem,
    sqlite3_result_error_toobig,
    sqlite3_sleep,
    sqlite3_soft_heap_limit,
    sqlite3_vfs_find,
    sqlite3_vfs_register,
    sqlite3_vfs_unregister,
    
    /*
     ** Added for 3.5.8
     */
    sqlite3_threadsafe,
    sqlite3_result_zeroblob,
    sqlite3_result_error_code,
    sqlite3_test_control,
    sqlite3_randomness,
    sqlite3_context_db_handle,
    
    /*
     ** Added for 3.6.0
     */
    sqlite3_extended_result_codes,
    sqlite3_limit,
    sqlite3_next_stmt,
    sqlite3_sql,
    sqlite3_status,
    
    /*
     ** Added for 3.7.4
     */
    sqlite3_backup_finish,
    sqlite3_backup_init,
    sqlite3_backup_pagecount,
    sqlite3_backup_remaining,
    sqlite3_backup_step,
#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    sqlite3_compileoption_get,
    sqlite3_compileoption_used,
#else
    0,
    0,
#endif
    sqlite3_create_function_v2,
    sqlite3_db_config,
    sqlite3_db_mutex,
    sqlite3_db_status,
    sqlite3_extended_errcode,
    sqlite3_log,
    sqlite3_soft_heap_limit64,
    sqlite3_sourceid,
    sqlite3_stmt_status,
    sqlite3_strnicmp,
#ifdef SQLITE_ENABLE_UNLOCK_NOTIFY
    sqlite3_unlock_notify,
#else
    0,
#endif
#ifndef SQLITE_OMIT_WAL
    sqlite3_wal_autocheckpoint,
    sqlite3_wal_checkpoint,
    sqlite3_wal_hook,
#else
    0,
    0,
    0,
#endif
    sqlite3_blob_reopen,
    sqlite3_vtab_config,
    sqlite3_vtab_on_conflict,
    sqlite3_close_v2,
    sqlite3_db_filename,
    sqlite3_db_readonly,
    sqlite3_db_release_memory,
    sqlite3_errstr,
    sqlite3_stmt_busy,
    sqlite3_stmt_readonly,
    sqlite3_stricmp,
    sqlite3_uri_boolean,
    sqlite3_uri_int64,
    sqlite3_uri_parameter,
    sqlite3_vsnprintf,
    sqlite3_wal_checkpoint_v2
};

/*
 ** Attempt to load an SQLite extension library contained in the file
 ** zFile.  The entry point is zProc.  zProc may be 0 in which case a
 ** default entry point name (sqlite3_extension_init) is used.  Use
 ** of the default name is recommended.
 **
 ** Return SQLITE_OK on success and SQLITE_ERROR if something goes wrong.
 **
 ** If an error occurs and pzErrMsg is not 0, then fill *pzErrMsg with
 ** error message text.  The calling function should free this memory
 ** by calling sqlite3DbFree(db, ).
 */
static int sqlite3LoadExtension(
                                sqlite3 *db,          /* Load the extension into this database connection */
                                const char *zFile,    /* Name of the shared library containing extension */
                                const char *zProc,    /* Entry point.  Use "sqlite3_extension_init" if 0 */
                                char **pzErrMsg       /* Put error message here if not 0 */
){
    sqlite3_vfs *pVfs = db->pVfs;
    void *handle;
    int (*xInit)(sqlite3*,char**,const sqlite3_api_routines*);
    char *zErrmsg = 0;
    const char *zEntry;
    char *zAltEntry = 0;
    void **aHandle;
    int nMsg = 300 + sqlite3Strlen30(zFile);
    int ii;
    
    /* Shared library endings to try if zFile cannot be loaded as written */
    static const char *azEndings[] = {
#if SQLITE_OS_WIN
        "dll"
#elif defined(__APPLE__)
        "dylib"
#else
        "so"
#endif
    };
    
    
    if( pzErrMsg ) *pzErrMsg = 0;
    
    /* Ticket #1863.  To avoid a creating security problems for older
     ** applications that relink against newer versions of SQLite, the
     ** ability to run load_extension is turned off by default.  One
     ** must call sqlite3_enable_load_extension() to turn on extension
     ** loading.  Otherwise you get the following error.
     */
    if( (db->flags & SQLITE_LoadExtension)==0 ){
        if( pzErrMsg ){
            *pzErrMsg = sqlite3_mprintf("not authorized");
        }
        return SQLITE_ERROR;
    }
    
    zEntry = zProc ? zProc : "sqlite3_extension_init";
    
    handle = sqlite3OsDlOpen(pVfs, zFile);
#if SQLITE_OS_UNIX || SQLITE_OS_WIN
    for(ii=0; ii<ArraySize(azEndings) && handle==0; ii++){
        char *zAltFile = sqlite3_mprintf("%s.%s", zFile, azEndings[ii]);
        if( zAltFile==0 ) return SQLITE_NOMEM;
        handle = sqlite3OsDlOpen(pVfs, zAltFile);
        sqlite3_free(zAltFile);
    }
#endif
    if( handle==0 ){
        if( pzErrMsg ){
            *pzErrMsg = zErrmsg = sqlite3_malloc(nMsg);
            if( zErrmsg ){
                sqlite3_snprintf(nMsg, zErrmsg,
                                 "unable to open shared library [%s]", zFile);
                sqlite3OsDlError(pVfs, nMsg-1, zErrmsg);
            }
        }
        return SQLITE_ERROR;
    }
    xInit = (int(*)(sqlite3*,char**,const sqlite3_api_routines*))
    sqlite3OsDlSym(pVfs, handle, zEntry);
    
    /* If no entry point was specified and the default legacy
     ** entry point name "sqlite3_extension_init" was not found, then
     ** construct an entry point name "sqlite3_X_init" where the X is
     ** replaced by the lowercase value of every ASCII alphabetic
     ** character in the filename after the last "/" upto the first ".",
     ** and eliding the first three characters if they are "lib".
     ** Examples:
     **
     **    /usr/local/lib/libExample5.4.3.so ==>  sqlite3_example_init
     **    C:/lib/mathfuncs.dll              ==>  sqlite3_mathfuncs_init
     */
    if( xInit==0 && zProc==0 ){
        int iFile, iEntry, c;
        int ncFile = sqlite3Strlen30(zFile);
        zAltEntry = sqlite3_malloc(ncFile+30);
        if( zAltEntry==0 ){
            sqlite3OsDlClose(pVfs, handle);
            return SQLITE_NOMEM;
        }
        memcpy(zAltEntry, "sqlite3_", 8);
        for(iFile=ncFile-1; iFile>=0 && zFile[iFile]!='/'; iFile--){}
        iFile++;
        if( sqlite3_strnicmp(zFile+iFile, "lib", 3)==0 ) iFile += 3;
        for(iEntry=8; (c = zFile[iFile])!=0 && c!='.'; iFile++){
            if( sqlite3Isalpha(c) ){
                zAltEntry[iEntry++] = (char)sqlite3UpperToLower[(unsigned)c];
            }
        }
        memcpy(zAltEntry+iEntry, "_init", 6);
        zEntry = zAltEntry;
        xInit = (int(*)(sqlite3*,char**,const sqlite3_api_routines*))
        sqlite3OsDlSym(pVfs, handle, zEntry);
    }
    if( xInit==0 ){
        if( pzErrMsg ){
            nMsg += sqlite3Strlen30(zEntry);
            *pzErrMsg = zErrmsg = sqlite3_malloc(nMsg);
            if( zErrmsg ){
                sqlite3_snprintf(nMsg, zErrmsg,
                                 "no entry point [%s] in shared library [%s]", zEntry, zFile);
                sqlite3OsDlError(pVfs, nMsg-1, zErrmsg);
            }
        }
        sqlite3OsDlClose(pVfs, handle);
        sqlite3_free(zAltEntry);
        return SQLITE_ERROR;
    }
    sqlite3_free(zAltEntry);
    if( xInit(db, &zErrmsg, &sqlite3Apis) ){
        if( pzErrMsg ){
            *pzErrMsg = sqlite3_mprintf("error during initialization: %s", zErrmsg);
        }
        sqlite3_free(zErrmsg);
        sqlite3OsDlClose(pVfs, handle);
        return SQLITE_ERROR;
    }
    
    /* Append the new shared library handle to the db->aExtension array. */
    aHandle = sqlite3DbMallocZero(db, sizeof(handle)*(db->nExtension+1));
    if( aHandle==0 ){
        return SQLITE_NOMEM;
    }
    if( db->nExtension>0 ){
        memcpy(aHandle, db->aExtension, sizeof(handle)*db->nExtension);
    }
    sqlite3DbFree(db, db->aExtension);
    db->aExtension = aHandle;
    
    db->aExtension[db->nExtension++] = handle;
    return SQLITE_OK;
}
SQLITE_API int sqlite3_load_extension(
                                      sqlite3 *db,          /* Load the extension into this database connection */
                                      const char *zFile,    /* Name of the shared library containing extension */
                                      const char *zProc,    /* Entry point.  Use "sqlite3_extension_init" if 0 */
                                      char **pzErrMsg       /* Put error message here if not 0 */
){
    int rc;
    sqlite3_mutex_enter(db->mutex);
    rc = sqlite3LoadExtension(db, zFile, zProc, pzErrMsg);
    rc = sqlite3ApiExit(db, rc);
    sqlite3_mutex_leave(db->mutex);
    return rc;
}

/*
 ** Call this routine when the database connection is closing in order
 ** to clean up loaded extensions
 */
SQLITE_PRIVATE void sqlite3CloseExtensions(sqlite3 *db){
    int i;
    assert( sqlite3_mutex_held(db->mutex) );
    for(i=0; i<db->nExtension; i++){
        sqlite3OsDlClose(db->pVfs, db->aExtension[i]);
    }
    sqlite3DbFree(db, db->aExtension);
}

/*
 ** Enable or disable extension loading.  Extension loading is disabled by
 ** default so as not to open security holes in older applications.
 */
SQLITE_API int sqlite3_enable_load_extension(sqlite3 *db, int onoff){
    sqlite3_mutex_enter(db->mutex);
    if( onoff ){
        db->flags |= SQLITE_LoadExtension;
    }else{
        db->flags &= ~SQLITE_LoadExtension;
    }
    sqlite3_mutex_leave(db->mutex);
    return SQLITE_OK;
}

#endif /* SQLITE_OMIT_LOAD_EXTENSION */

/*
 ** The auto-extension code added regardless of whether or not extension
 ** loading is supported.  We need a dummy sqlite3Apis pointer for that
 ** code if regular extension loading is not available.  This is that
 ** dummy pointer.
 */
#ifdef SQLITE_OMIT_LOAD_EXTENSION
static const sqlite3_api_routines sqlite3Apis = { 0 };
#endif


/*
 ** The following object holds the list of automatically loaded
 ** extensions.
 **
 ** This list is shared across threads.  The SQLITE_MUTEX_STATIC_MASTER
 ** mutex must be held while accessing this list.
 */
typedef struct sqlite3AutoExtList sqlite3AutoExtList;
static SQLITE_WSD struct sqlite3AutoExtList {
    int nExt;              /* Number of entries in aExt[] */
    void (**aExt)(void);   /* Pointers to the extension init functions */
} sqlite3Autoext = { 0, 0 };

/* The "wsdAutoext" macro will resolve to the autoextension
 ** state vector.  If writable static data is unsupported on the target,
 ** we have to locate the state vector at run-time.  In the more common
 ** case where writable static data is supported, wsdStat can refer directly
 ** to the "sqlite3Autoext" state vector declared above.
 */
#ifdef SQLITE_OMIT_WSD
# define wsdAutoextInit \
sqlite3AutoExtList *x = &GLOBAL(sqlite3AutoExtList,sqlite3Autoext)
# define wsdAutoext x[0]
#else
# define wsdAutoextInit
# define wsdAutoext sqlite3Autoext
#endif


/*
 ** Register a statically linked extension that is automatically
 ** loaded by every new database connection.
 */
SQLITE_API int sqlite3_auto_extension(void (*xInit)(void)){
    int rc = SQLITE_OK;
#ifndef SQLITE_OMIT_AUTOINIT
    rc = sqlite3_initialize();
    if( rc ){
        return rc;
    }else
#endif
    {
        int i;
#if SQLITE_THREADSAFE
        sqlite3_mutex *mutex = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_MASTER);
#endif
        wsdAutoextInit;
        sqlite3_mutex_enter(mutex);
        for(i=0; i<wsdAutoext.nExt; i++){
            if( wsdAutoext.aExt[i]==xInit ) break;
        }
        if( i==wsdAutoext.nExt ){
            int nByte = (wsdAutoext.nExt+1)*sizeof(wsdAutoext.aExt[0]);
            void (**aNew)(void);
            aNew = sqlite3_realloc(wsdAutoext.aExt, nByte);
            if( aNew==0 ){
                rc = SQLITE_NOMEM;
            }else{
                wsdAutoext.aExt = aNew;
                wsdAutoext.aExt[wsdAutoext.nExt] = xInit;
                wsdAutoext.nExt++;
            }
        }
        sqlite3_mutex_leave(mutex);
        assert( (rc&0xff)==rc );
        return rc;
    }
}

/*
 ** Cancel a prior call to sqlite3_auto_extension.  Remove xInit from the
 ** set of routines that is invoked for each new database connection, if it
 ** is currently on the list.  If xInit is not on the list, then this
 ** routine is a no-op.
 **
 ** Return 1 if xInit was found on the list and removed.  Return 0 if xInit
 ** was not on the list.
 */
SQLITE_API int sqlite3_cancel_auto_extension(void (*xInit)(void)){
#if SQLITE_THREADSAFE
    sqlite3_mutex *mutex = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_MASTER);
#endif
    int i;
    int n = 0;
    wsdAutoextInit;
    sqlite3_mutex_enter(mutex);
    for(i=wsdAutoext.nExt-1; i>=0; i--){
        if( wsdAutoext.aExt[i]==xInit ){
            wsdAutoext.nExt--;
            wsdAutoext.aExt[i] = wsdAutoext.aExt[wsdAutoext.nExt];
            n++;
            break;
        }
    }
    sqlite3_mutex_leave(mutex);
    return n;
}

/*
 ** Reset the automatic extension loading mechanism.
 */
SQLITE_API void sqlite3_reset_auto_extension(void){
#ifndef SQLITE_OMIT_AUTOINIT
    if( sqlite3_initialize()==SQLITE_OK )
#endif
    {
#if SQLITE_THREADSAFE
        sqlite3_mutex *mutex = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_MASTER);
#endif
        wsdAutoextInit;
        sqlite3_mutex_enter(mutex);
        sqlite3_free(wsdAutoext.aExt);
        wsdAutoext.aExt = 0;
        wsdAutoext.nExt = 0;
        sqlite3_mutex_leave(mutex);
    }
}

/*
 ** Load all automatic extensions.
 **
 ** If anything goes wrong, set an error in the database connection.
 */
SQLITE_PRIVATE void sqlite3AutoLoadExtensions(sqlite3 *db){
    int i;
    int go = 1;
    int rc;
    int (*xInit)(sqlite3*,char**,const sqlite3_api_routines*);
    
    wsdAutoextInit;
    if( wsdAutoext.nExt==0 ){
        /* Common case: early out without every having to acquire a mutex */
        return;
    }
    for(i=0; go; i++){
        char *zErrmsg;
#if SQLITE_THREADSAFE
        sqlite3_mutex *mutex = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_MASTER);
#endif
        sqlite3_mutex_enter(mutex);
        if( i>=wsdAutoext.nExt ){
            xInit = 0;
            go = 0;
        }else{
            xInit = (int(*)(sqlite3*,char**,const sqlite3_api_routines*))
            wsdAutoext.aExt[i];
        }
        sqlite3_mutex_leave(mutex);
        zErrmsg = 0;
        if( xInit && (rc = xInit(db, &zErrmsg, &sqlite3Apis))!=0 ){
            sqlite3Error(db, rc,
                         "automatic extension loading failed: %s", zErrmsg);
            go = 0;
        }
        sqlite3_free(zErrmsg);
    }
}

/************** End of loadext.c *********************************************/