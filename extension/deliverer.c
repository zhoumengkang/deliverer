/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: mengkang <i@mengkang.net>                                    |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_deliverer.h"
#include "main/SAPI.h"

#if PHP_VERSION_ID < 50400
#define OP1_FUNCTION_PTR(n) (&(n)->op1.u.constant)
#elif PHP_VERSION_ID < 70000
#define OP1_FUNCTION_PTR(n) ((n)->op1.zv)
#else
#endif

static int le_deliverer;

static int deliverer_stat = -1;

static char *deliverer_watch = NULL;

static FILE *fp;

static char *get_function_name(zend_function *fbc) /* {{{ */
{
    char *method_or_function_name = NULL;

#if PHP_VERSION_ID < 70000
    const char *function_name = fbc->common.function_name;

    if (function_name == NULL)
    {
        return NULL;
    }

    zend_class_entry *scope = fbc->common.scope;

    if (scope != NULL)
    {
        const char *class_name = scope->name;
        int len = strlen(class_name) + strlen("::") + strlen(function_name) + 1;
        method_or_function_name = (char *)emalloc(len);

        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, class_name);
        strcat(method_or_function_name, "::");
        strcat(method_or_function_name, function_name);
    }
    else
    {
        int len = strlen(function_name) + 1;
        method_or_function_name = (char *)emalloc(len);
        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, function_name);
    }
#else
    zend_string *function_name = fbc->common.function_name;
    zend_class_entry *scope = fbc->common.scope;
    char *class_name = NULL;

    if (scope != NULL)
    {
        class_name = ZSTR_VAL(scope->name);
        int len = strlen(class_name) + strlen("::") + ZSTR_LEN(function_name) + 1;
        method_or_function_name = (char *)emalloc(len);
        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, class_name);
        strcat(method_or_function_name, "::");
        strcat(method_or_function_name, ZSTR_VAL(function_name));
    }
    else
    {
        int len = ZSTR_LEN(function_name) + 1;
        method_or_function_name = (char *)emalloc(len);
        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, ZSTR_VAL(function_name));
    }
#endif

    return method_or_function_name;
}
/* }}} */

#if PHP_VERSION_ID < 70000
static zend_function *get_function_from_opline(zend_op *opline) /* {{{ */
{
    zend_function *fbc;

    zval *function_name = OP1_FUNCTION_PTR(opline);

    if (Z_STRVAL_P(function_name) == NULL)
    {
        return NULL;
    }

    if (zend_hash_find(EG(function_table), Z_STRVAL_P(function_name), Z_STRLEN_P(function_name) + 1, (void **)&fbc) ==
        FAILURE)
    {
        return NULL;
    }

    return fbc;
}
/* }}} */
#endif

/* {{{ php_deliverer_log_handler(zend_execute_data *execute_data)
 */
#if PHP_VERSION_ID < 70000
static int php_deliverer_log_handler(ZEND_OPCODE_HANDLER_ARGS)
#else
static int php_deliverer_log_handler(zend_execute_data *execute_data)
#endif
{
    if (deliverer_stat == -1)
    {
        return ZEND_USER_OPCODE_DISPATCH;
    }

    zend_function *fbc;

#if PHP_VERSION_ID < 70000
    #if PHP_VERSION_ID < 50500
    if (execute_data->fbc != NULL)
    {
        fbc = execute_data->fbc; // function inner call
    }
    #else
    if (execute_data->call != NULL && execute_data->call->fbc != NULL)
    {
        fbc = execute_data->call->fbc; // function inner call
    }
    #endif
    if (fbc == NULL)
    {
        fbc = get_function_from_opline(execute_data->opline);
    }
#else
    if (execute_data->call != NULL && execute_data->call->func != NULL)
    {
        fbc = execute_data->call->func;
    }
#endif

    if (fbc == NULL)
    {
        return ZEND_USER_OPCODE_DISPATCH;
    }

    if (fbc->type != ZEND_USER_FUNCTION)
    {
        return ZEND_USER_OPCODE_DISPATCH;
    }

    char *method_or_function_name = get_function_name(fbc);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    const char *filename = zend_get_executed_filename();
    int lineno = zend_get_executed_lineno();

    //format time|current_p|prev_p|function|filename|line
    fprintf(fp, "%ld|%p|%p|%s|%s|%d\n", tv.tv_sec * 1000000 + tv.tv_usec, execute_data, execute_data->prev_execute_data,
            method_or_function_name, filename, lineno);

    efree(method_or_function_name);

    return ZEND_USER_OPCODE_DISPATCH;
}
/* }}} */

static void set_deliverer_stat() /* {{{ */
{
    deliverer_stat = access("/tmp/deliverer/config/stat", F_OK);
}
/* }}} */

static void get_deliverer_watch() /* {{{ */
{
    const char *watch = "/tmp/deliverer/config/watch";

    if (access(watch,R_OK) != 0) {
        return;
    }

    struct stat statbuf;

    stat(watch,&statbuf);
    int size = statbuf.st_size;

    deliverer_watch = (char *)emalloc(size+1);
    memset(deliverer_watch, 0, size+1);

    FILE *fp;
    if ((fp = fopen(watch, "r")) == NULL){
        return;
    }

    fgets(deliverer_watch, size, fp);
}
/* }}} */

static char *build_deliverer_cli_argv() /* {{{ */
{
    char *cli_argv_string = NULL;

    int len = 0;
    int i;

    if (SG(request_info).argc > 0)
    {

        for (i = 0; i < SG(request_info).argc; i++)
        {
            len += strlen(SG(request_info).argv[i]);
        }

        len += SG(request_info).argc;

        cli_argv_string = (char *)emalloc(len + SG(request_info).argc);

        memset(cli_argv_string, 0, len);

        for (i = 0; i < SG(request_info).argc; i++)
        {
            strcat(cli_argv_string, SG(request_info).argv[i]);
            strcat(cli_argv_string, " ");
        }
    }

    return cli_argv_string;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(deliverer)
{

#if PHP_VERSION_ID < 70000
#else
    zend_set_user_opcode_handler(ZEND_DO_UCALL, php_deliverer_log_handler);
#endif

    zend_set_user_opcode_handler(ZEND_DO_FCALL_BY_NAME, php_deliverer_log_handler);
    zend_set_user_opcode_handler(ZEND_DO_FCALL, php_deliverer_log_handler);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(deliverer)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(deliverer)
{
#if defined(COMPILE_DL_DELIVERER) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    set_deliverer_stat();

    if (deliverer_stat == -1)
    {
        return SUCCESS;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    long ts = tv.tv_sec * 1000000 + tv.tv_usec;

    char str[128] = {0};
    sprintf(str, "/tmp/deliverer/log/%d-%ld.log", getpid(), ts);
    fp = fopen(str, "a+");

    if (strcmp(sapi_module.name, "cli") == 0)
    {

        char *cli_argv_string = build_deliverer_cli_argv();

        fprintf(fp, "---\n%d-%ld %s %s\n", getpid(), tv.tv_sec * 1000000 + tv.tv_usec, sapi_module.name,
                cli_argv_string);

        if (cli_argv_string != NULL)
        {
            efree(cli_argv_string);
        }
    }
    else
    {
        fprintf(fp, "---\n%d-%ld %s %s %s %s\n", getpid(), tv.tv_sec * 1000000 + tv.tv_usec, sapi_module.name,
                SG(request_info).request_method, SG(request_info).request_uri, SG(request_info).query_string);
    }

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(deliverer)
{
    if (deliverer_stat == -1)
    {
        return SUCCESS;
    }

    fprintf(fp, "---end---");
    fclose(fp);

    if (deliverer_watch != NULL)
    {
        efree(deliverer_watch);
    }

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(deliverer)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "deliverer support", "enabled");
    php_info_print_table_end();
}
/* }}} */

/* {{{ deliverer_functions[]
 *
 */
const zend_function_entry deliverer_functions[] = {
	PHP_FE_END
};
/* }}} */

/* {{{ deliverer_module_entry
 */
zend_module_entry deliverer_module_entry = {
	STANDARD_MODULE_HEADER,
	"deliverer",
	deliverer_functions,
	PHP_MINIT(deliverer),
	PHP_MSHUTDOWN(deliverer),
	PHP_RINIT(deliverer),
	PHP_RSHUTDOWN(deliverer),
	PHP_MINFO(deliverer),
	PHP_DELIVERER_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_DELIVERER
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(deliverer)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
