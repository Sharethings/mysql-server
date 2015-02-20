#ifndef SQL_AUDIT_INCLUDED
#define SQL_AUDIT_INCLUDED

/* Copyright (c) 2007, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */


#include <my_global.h>

#include <mysql/plugin_audit.h>
#include "sql_class.h"
#include "sql_rewrite.h"

extern unsigned long mysql_global_audit_mask[];


extern void mysql_audit_initialize();
extern void mysql_audit_finalize();


extern void mysql_audit_init_thd(THD *thd);
extern void mysql_audit_free_thd(THD *thd);
extern void mysql_audit_acquire_plugins(THD *thd, uint event_class);


#ifndef EMBEDDED_LIBRARY
extern void mysql_audit_notify(THD *thd, uint event_class,
                               uint event_subtype, ...);
#else
#define mysql_audit_notify(...)
#endif
extern void mysql_audit_release(THD *thd);

#define MAX_USER_HOST_SIZE 512
static inline size_t make_user_name(THD *thd, char *buf)
{
  Security_context *sctx= thd->security_context();
  LEX_CSTRING sctx_user= sctx->user();
  LEX_CSTRING sctx_host= sctx->host();
  LEX_CSTRING sctx_ip= sctx->ip();
  LEX_CSTRING sctx_priv_user= sctx->priv_user();
  return static_cast<size_t>(strxnmov(buf, MAX_USER_HOST_SIZE,
                                      sctx_priv_user.str[0] ?
                                        sctx_priv_user.str : "", "[",
                                      sctx_user.length ? sctx_user.str :
                                                         "", "] @ ",
                                      sctx_host.length ? sctx_host.str :
                                                         "", " [",
                                      sctx_ip.length ? sctx_ip.str : "", "]",
                                      NullS)
                             - buf);
}

/**
  Call audit plugins of GENERAL audit class, MYSQL_AUDIT_GENERAL_LOG subtype.
  
  @param[in] thd
  @param[in] time             time that event occurred
  @param[in] user             User name
  @param[in] userlen          User name length
  @param[in] cmd              Command name
  @param[in] cmdlen           Command name length
  @param[in] query            Query string
  @param[in] querylen         Query string length
*/
 
static inline
void mysql_audit_general_log(THD *thd, const char *cmd, size_t cmdlen)
{
#ifndef EMBEDDED_LIBRARY
  if (mysql_global_audit_mask[0] & MYSQL_AUDIT_GENERAL_CLASSMASK)
  {
    MYSQL_LEX_STRING sql_command, ip, host, external_user;
    LEX_CSTRING query= EMPTY_CSTR;
    static MYSQL_LEX_STRING empty= { C_STRING_WITH_LEN("") };
    char user_buff[MAX_USER_HOST_SIZE + 1];
    const char *user= user_buff;
    size_t userlen= make_user_name(thd, user_buff);
    time_t time= (time_t) thd->start_time.tv_sec;
    int error_code= 0;

    if (thd)
    {
      if (!thd->rewritten_query.length())
        mysql_rewrite_query(thd);
      if (thd->rewritten_query.length())
      {
        query.str= thd->rewritten_query.ptr();
        query.length= thd->rewritten_query.length();
      }
      else
        query= thd->query();
      Security_context *sctx= thd->security_context();
      LEX_CSTRING sctx_host= sctx->host();
      LEX_CSTRING sctx_ip= sctx->ip();
      LEX_CSTRING sctx_external_user= sctx->external_user();
      ip.str= (char *) sctx_ip.str;
      ip.length= sctx_ip.length;
      host.str= (char *) sctx_host.str;
      host.length= sctx_host.length;
      external_user.str= (char *) sctx_external_user.str;
      external_user.length= sctx_external_user.length;
      sql_command.str= (char *) sql_statement_names[thd->lex->sql_command].str;
      sql_command.length= sql_statement_names[thd->lex->sql_command].length;
    }
    else
    {
      ip= empty;
      host= empty;
      external_user= empty;
      sql_command= empty;
    }
    const CHARSET_INFO *clientcs= thd ? thd->variables.character_set_client
      : global_system_variables.character_set_client;

    mysql_audit_notify(thd, MYSQL_AUDIT_GENERAL_CLASS, MYSQL_AUDIT_GENERAL_LOG,
                       error_code, time, user, userlen, cmd, cmdlen, query.str,
                       query.length, clientcs,
                       static_cast<ha_rows>(0), /* general_rows */
                       sql_command, host, external_user, ip);

  }
#endif
}


/**
  Call audit plugins of GENERAL audit class.
  event_subtype should be set to one of:
    MYSQL_AUDIT_GENERAL_ERROR
    MYSQL_AUDIT_GENERAL_RESULT
    MYSQL_AUDIT_GENERAL_STATUS
  
  @param[in] thd
  @param[in] event_subtype    Type of general audit event.
  @param[in] error_code       Error code
  @param[in] msg              Message
*/
static inline
void mysql_audit_general(THD *thd, uint event_subtype,
                         int error_code, const char *msg)
{
#ifndef EMBEDDED_LIBRARY
  if (mysql_global_audit_mask[0] & MYSQL_AUDIT_GENERAL_CLASSMASK)
  {
    time_t time= my_time(0);
    size_t msglen= msg ? strlen(msg) : 0;
    size_t userlen;
    const char *user;
    char user_buff[MAX_USER_HOST_SIZE];
    LEX_CSTRING query= EMPTY_CSTR;
    const CHARSET_INFO *query_charset= thd->charset();
    MYSQL_LEX_STRING ip, host, external_user, sql_command;
    ha_rows rows;
    Security_context *sctx;
    LEX_CSTRING sctx_host, sctx_ip, sctx_external_user;
    static MYSQL_LEX_STRING empty= { C_STRING_WITH_LEN("") };

    if (thd)
    {
      if (!thd->rewritten_query.length())
        mysql_rewrite_query(thd);
      if (thd->rewritten_query.length())
      {
        query.str= thd->rewritten_query.ptr();
        query.length= thd->rewritten_query.length();
        query_charset= thd->rewritten_query.charset();
      }
      else
        query= thd->query();
      user= user_buff;
      userlen= make_user_name(thd, user_buff);
      sctx= thd->security_context();
      rows= thd->get_stmt_da()->current_row_for_condition();
      sctx_ip= sctx->ip();
      ip.str= (char *) sctx_ip.str;
      ip.length= sctx_ip.length;
      sctx_host= sctx->host();
      host.str= (char *) sctx_host.str;
      host.length= sctx_host.length;
      sctx_external_user= sctx->external_user();
      external_user.str= (char *) sctx_external_user.str;
      external_user.length= sctx_external_user.length;
      sql_command.str= (char *) sql_statement_names[thd->lex->sql_command].str;
      sql_command.length= sql_statement_names[thd->lex->sql_command].length;
    }
    else
    {
      user= 0;
      userlen= 0;
      ip= empty;
      host= empty;
      external_user= empty;
      sql_command= empty;
      rows= 0;
    }

    mysql_audit_notify(thd, MYSQL_AUDIT_GENERAL_CLASS, event_subtype,
                       error_code, time, user, userlen, msg, msglen,
                       query.str, query.length, query_charset, rows,
                       sql_command, host, external_user, ip);
  }
#endif
}

#define MYSQL_AUDIT_NOTIFY_CONNECTION_CONNECT(thd) mysql_audit_notify(\
  (thd), MYSQL_AUDIT_CONNECTION_CLASS, MYSQL_AUDIT_CONNECTION_CONNECT,\
  (thd)->get_stmt_da()->is_error() ? (thd)->get_stmt_da()->mysql_errno() : 0,\
  (thd)->thread_id(), (thd)->security_context()->user().str,\
  (thd)->security_context()->user().length,\
  (thd)->security_context()->priv_user().str,\
  (thd)->security_context()->priv_user().length,\
  (thd)->security_context()->external_user().str,\
  (thd)->security_context()->external_user().length,\
  (thd)->security_context()->proxy_user().str,\
  (thd)->security_context()->proxy_user().length,\
  (thd)->security_context()->host().str,\
  (thd)->security_context()->host().length,\
  (thd)->security_context()->ip().str,\
  (thd)->security_context()->ip().length,\
  (thd)->db().str, (thd)->db().length)

#define MYSQL_AUDIT_NOTIFY_CONNECTION_DISCONNECT(thd, errcode)\
  mysql_audit_notify(\
  (thd), MYSQL_AUDIT_CONNECTION_CLASS, MYSQL_AUDIT_CONNECTION_DISCONNECT,\
  (errcode), (thd)->thread_id(),\
  (thd)->security_context()->user().str,\
  (thd)->security_context()->user().length,\
  (thd)->security_context()->priv_user().str,\
  (thd)->security_context()->priv_user().length,\
  (thd)->security_context()->external_user().str,\
  (thd)->security_context()->external_user().length,\
  (thd)->security_context()->proxy_user().str,\
  (thd)->security_context()->proxy_user().length,\
  (thd)->security_context()->host().str,\
  (thd)->security_context()->host().length,\
  (thd)->security_context()->ip().str,\
  (thd)->security_context()->ip().length,\
  (thd)->db().str, (thd)->db().length)


#define MYSQL_AUDIT_NOTIFY_CONNECTION_CHANGE_USER(thd) mysql_audit_notify(\
  (thd), MYSQL_AUDIT_CONNECTION_CLASS, MYSQL_AUDIT_CONNECTION_CHANGE_USER,\
  (thd)->get_stmt_da()->is_error() ? (thd)->get_stmt_da()->mysql_errno() : 0,\
  (thd)->thread_id(), (thd)->security_context()->user().str,\
  (thd)->security_context()->user().length,\
  (thd)->security_context()->priv_user().str,\
  (thd)->security_context()->priv_user().length,\
  (thd)->security_context()->external_user().str,\
  (thd)->security_context()->external_user().length,\
  (thd)->security_context()->proxy_user().str,\
  (thd)->security_context()->proxy_user().length,\
  (thd)->security_context()->host().str,\
  (thd)->security_context()->host().length,\
  (thd)->security_context()->ip().str,\
  (thd)->security_context()->ip().length,\
  (thd)->db().str, (thd)->db().length)

#endif /* SQL_AUDIT_INCLUDED */
