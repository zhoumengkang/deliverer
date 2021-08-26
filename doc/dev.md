# PHP 5
> Zend/zend_compile.h

```c
typedef union _zend_function {
	zend_uchar type;	/* MUST be the first element of this struct! */

	struct {
		zend_uchar type;  /* never used */
		const char *function_name;
		zend_class_entry *scope;
		zend_uint fn_flags;
		union _zend_function *prototype;
		zend_uint num_args;
		zend_uint required_num_args;
		zend_arg_info *arg_info;
	} common;

	zend_op_array op_array;
	zend_internal_function internal_function;
} zend_function;
```
```c
typedef struct _zend_arg_info {
	const char *name;
	zend_uint name_len;
	const char *class_name;
	zend_uint class_name_len;
	zend_uchar type_hint;
	zend_bool allow_null;
	zend_bool pass_by_reference;
} zend_arg_info;
```

> ext/standard/php_var.h

```c
PHPAPI void php_var_export(zval **struc, int level TSRMLS_DC);
```

# PHP7
```c
struct _zend_execute_data {
	const zend_op       *opline;           /* executed opline                */
	zend_execute_data   *call;             /* current call                   */
	zval                *return_value;
	zend_function       *func;             /* executed function              */
	zval                 This;             /* this + call_info + num_args    */
	zend_execute_data   *prev_execute_data;
	zend_array          *symbol_table;
#if ZEND_EX_USE_RUN_TIME_CACHE
	void               **run_time_cache;   /* cache op_array->run_time_cache */
#endif
#if ZEND_EX_USE_LITERALS
	zval                *literals;         /* cache op_array->literals       */
#endif
};
```