dnl $Id$
dnl config.m4 for extension deliverer

PHP_ARG_WITH(deliverer, for deliverer support,
[  --with-deliverer             Include deliverer support])

if test "$PHP_DELIVERER" != "no"; then
  PHP_NEW_EXTENSION(deliverer, deliverer.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
