dnl $Id$

dnl Wrapper for AC_PATH_PROG that kills configure script if program is missing
AC_DEFUN([MY_PATH_PROG],
[AC_PATH_PROG([$1],[$1],NOT_FOUND,$PATH:/sbin:/usr/sbin:/usr/local/sbin)
test $[$1] == NOT_FOUND && AC_MSG_ERROR([program $1 is required])])

dnl Some standard stuff
AC_INIT()
AC_PROG_CC
AC_LANG_C
AC_PROG_CXX
AC_LANG_CPLUSPLUS

dnl Check for all required utilities/tools
MY_PATH_PROG([wget])
MY_PATH_PROG([mkdir])
MY_PATH_PROG([rm])
MY_PATH_PROG([tar])
MY_PATH_PROG([unzip])
MY_PATH_PROG([cp])
MY_PATH_PROG([find])
MY_PATH_PROG([cpio])
MY_PATH_PROG([gzip])
MY_PATH_PROG([cd])
MY_PATH_PROG([echo])
MY_PATH_PROG([sfdisk])
MY_PATH_PROG([cat])
MY_PATH_PROG([test])
MY_PATH_PROG([exit])
MY_PATH_PROG([read])
MY_PATH_PROG([grep])
MY_PATH_PROG([dd])
MY_PATH_PROG([mke2fs])
MY_PATH_PROG([ln])
MY_PATH_PROG([mount])
MY_PATH_PROG([umount])
MY_PATH_PROG([sed])
MY_PATH_PROG([mknod])
MY_PATH_PROG([touch])
MY_PATH_PROG([rpm2cpio])
MY_PATH_PROG([install])
MY_PATH_PROG([patch])
MY_PATH_PROG([chown])
MY_PATH_PROG([chmod])
MY_PATH_PROG([md5sum])
MY_PATH_PROG([sha256sum])
MY_PATH_PROG([blockdev])
MY_PATH_PROG([sleep])

