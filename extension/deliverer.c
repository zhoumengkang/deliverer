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
  | Author: i@mengkang.net                                               |
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

/* If you declare any globals in php_deliverer.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(deliverer)
*/

/* True global resources - no need for thread safety here */
static int le_deliverer;

static FILE *fp;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("deliverer.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_deliverer_globals, deliverer_globals)
    STD_PHP_INI_ENTRY("deliverer.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_deliverer_globals, deliverer_globals)
PHP_INI_END()
*/
/* }}} */


PHP_FUNCTION(confirm_deliverer_compiled) /* {{{ */
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "deliverer", arg);

	RETURN_STR(strg);
}
/* }}} */


/* {{{ php_deliverer_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_deliverer_init_globals(zend_deliverer_globals *deliverer_globals)
{
	deliverer_globals->global_value = 0;
	deliverer_globals->global_string = NULL;
}
*/
/* }}} */

static char *get_function_name(zend_execute_data *call) /* {{{ */
{
    if (!call) {
        return NULL;
    }

    zend_function    *fbc                     = call->func;
    zend_string      *function_name           = fbc->common.function_name;
    zend_class_entry *scope                   = fbc->common.scope;
    char             *class_name              = NULL;
    char             *method_or_function_name = NULL;
    if (scope != NULL) {
        class_name = ZSTR_VAL(scope->name);
        int len    = strlen(class_name) + strlen("::") + ZSTR_LEN(function_name) + 1;
        method_or_function_name = (char *) emalloc(len);
        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, class_name);
        strcat(method_or_function_name, "::");
        strcat(method_or_function_name, ZSTR_VAL(function_name));
    } else {
        int len = ZSTR_LEN(function_name) + 1;
        method_or_function_name = (char *) emalloc(len);
        memset(method_or_function_name, 0, len);
        strcat(method_or_function_name, ZSTR_VAL(function_name));
    }

    return method_or_function_name;
}
/* }}} */

static int php_deliverer_log_handler(zend_execute_data *execute_data) /* {{{ */
{
    zend_execute_data *call = execute_data->call;
    zend_function     *fbc  = call->func;

    if (fbc->type != ZEND_USER_FUNCTION) {
        return ZEND_USER_OPCODE_DISPATCH;
    }
    char *method_or_function_name = get_function_name(call);
    printf("%s\n", method_or_function_name);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    const char *filename = zend_get_executed_filename();
    int        lineno    = zend_get_executed_lineno();


    //format pid-time|current_p|prev_p|function|filename|line
    fprintf(fp, "%ld|%p|%p|%s|%s|%d\n", tv.tv_sec * 1000000 + tv.tv_usec, execute_data, execute_data->prev_execute_data,
            method_or_function_name, filename, lineno);

    efree(method_or_function_name);

    return ZEND_USER_OPCODE_DISPATCH;
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(deliverer)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
    zend_set_user_opcode_handler(ZEND_DO_UCALL, php_deliverer_log_handler);
    zend_set_user_opcode_handler(ZEND_DO_FCALL_BY_NAME, php_deliverer_log_handler);
    zend_set_user_opcode_handler(ZEND_DO_FCALL, php_deliverer_log_handler);
	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(deliverer)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(deliverer)
{
#if defined(COMPILE_DL_DELIVERER) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	struct timeval tv;
    gettimeofday(&tv, NULL);

	long ts = tv.tv_sec * 1000000 + tv.tv_usec;

	char str[128] = {0};
    sprintf(str, "/tmp/deliverer/%d-%ld.log", getpid(), ts);
    fp = fopen(str, "a+");
    fprintf(fp, "---\n%d-%ld %s %s %s?%s\n",getpid() , tv.tv_sec * 1000000 + tv.tv_usec, sapi_module.name, SG(request_info).request_method, SG(request_info).request_uri, SG(request_info).query_string);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(deliverer)
{
	fprintf(fp, "---end---\n");
    fclose(fp);
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

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ deliverer_functions[]
 *
 * Every user visible function must have an entry in deliverer_functions[].
 */
const zend_function_entry deliverer_functions[] = {
	PHP_FE(confirm_deliverer_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in deliverer_functions[] */
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
	PHP_RINIT(deliverer),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(deliverer),	/* Replace with NULL if there's nothing to do at request end */
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
