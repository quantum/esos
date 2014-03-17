dnl $Id$

dnl Wrapper for AC_PATH_PROG that kills configure script if program is missing
AC_DEFUN([MY_PATH_PROG],
	[AC_PATH_PROG([$1],[$1],NOT_FOUND,$PATH:/sbin:/usr/sbin:/usr/local/sbin)
	test $[$1] == NOT_FOUND && AC_MSG_ERROR([program $1 is required])])

AC_INIT()

dnl Must be a 64-bit Linux host
AC_MSG_CHECKING([if this is a x86-64 Linux host])
if test "`uname -o`" == "GNU/Linux" && test "`uname -m`" == "x86_64"
then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([this system must be GNU/Linux x86_64])
fi

dnl Compiler checks
AC_PROG_CC
AC_LANG_C
AC_PROG_CXX
AC_LANG_CPLUSPLUS

dnl Check for all required utilities/tools
MY_PATH_PROG([bash])
MY_PATH_PROG([wget])
MY_PATH_PROG([mkdir])
MY_PATH_PROG([rm])
MY_PATH_PROG([tar])
MY_PATH_PROG([cp])
MY_PATH_PROG([find])
MY_PATH_PROG([cpio])
MY_PATH_PROG([gzip])
MY_PATH_PROG([echo])
MY_PATH_PROG([sfdisk])
MY_PATH_PROG([cat])
MY_PATH_PROG([test])
MY_PATH_PROG([grep])
MY_PATH_PROG([dd])
MY_PATH_PROG([mke2fs])
MY_PATH_PROG([ln])
MY_PATH_PROG([mount])
MY_PATH_PROG([umount])
MY_PATH_PROG([sed])
MY_PATH_PROG([mknod])
MY_PATH_PROG([touch])
MY_PATH_PROG([install])
MY_PATH_PROG([patch])
MY_PATH_PROG([chown])
MY_PATH_PROG([chmod])
MY_PATH_PROG([md5sum])
MY_PATH_PROG([sha256sum])
MY_PATH_PROG([sleep])
MY_PATH_PROG([losetup])
MY_PATH_PROG([kpartx])
MY_PATH_PROG([xargs])
MY_PATH_PROG([openssl])

dnl Setup build directories
if test x"${PWD}" != x; then
	AC_MSG_NOTICE([setting up build directories...])
	AC_SUBST(cwd, ${PWD})
	work_dir="${PWD}/work"
	${mkdir} -p ${work_dir} || exit 1
	AC_SUBST(work_dir, ${work_dir})
	distfiles_dir="${work_dir}/distfiles"
	${mkdir} -p ${distfiles_dir} || exit 1
	AC_SUBST(distfiles_dir, ${distfiles_dir})
	build_dir="${work_dir}/build"
	${mkdir} -p ${build_dir} || exit 1
	AC_SUBST(build_dir, ${build_dir})
	image_dir="${work_dir}/image"
	${mkdir} -p ${image_dir} || exit 1
	AC_SUBST(image_dir, ${image_dir})
	initramfs_dir="${work_dir}/initramfs"
	${mkdir} -p ${initramfs_dir} || exit 1
	AC_SUBST(initramfs_dir, ${initramfs_dir})
	mount_dir="${work_dir}/mnt"
	${mkdir} -p ${mount_dir} || exit 1
	AC_SUBST(mount_dir, ${mount_dir})
else
	AC_MSG_ERROR([invalid current working directory])
fi

dnl Create directory structure for the image
if test -d "${image_dir}"; then
	AC_MSG_NOTICE([creating image directory structure...])
	${mkdir} -p ${image_dir}/{etc,bin,sbin,dev,proc,sys,root,home} || exit 1
        ${mkdir} -p ${image_dir}/boot/grub || exit 1
        ${mkdir} -p ${image_dir}/mnt/{root,conf,logs,vdisks,mhvtl} || exit 1
        ${mkdir} -p ${image_dir}/lib/firmware || exit 1
        ${mkdir} -p ${image_dir}/usr/{bin,sbin,libexec,lib} || exit 1
        ${mkdir} -p ${image_dir}/usr/local/{bin,sbin} || exit 1
        ${mkdir} -p ${image_dir}/opt/{bin,sbin,lib} || exit 1
        ${mkdir} -p -m 1777 ${image_dir}/tmp || exit 1
        ${mkdir} -p ${image_dir}/var/{spool,lock,run,state,cache,log} || exit 1
        ${mkdir} -p -m 755 ${image_dir}/var/empty || exit 1
        ${mkdir} -p -m 1777 ${image_dir}/var/tmp || exit 1
        ${mkdir} -p -m 710 ${image_dir}/var/cron || exit 1
        ${mkdir} -p -m 700 ${image_dir}/var/cron/tabs || exit 1
        ${mkdir} -p ${image_dir}/var/lib/corosync || exit 1
        ${mkdir} -p ${image_dir}/var/lib/pacemaker/{cores,pengine,cib,blackbox} || exit 1
        ${mkdir} -p ${image_dir}/var/lib/pacemaker/cores/{root,hacluster} || exit 1
        ${mkdir} -p ${image_dir}/var/run/crm || exit 1
        ${mkdir} -p ${image_dir}/var/lib/heartbeat/{cores,lrm} || exit 1
        ${ln} -sf lib ${image_dir}/lib64 || exit 1
        ${ln} -sf lib ${image_dir}/usr/lib64 || exit 1
        ${mkdir} -p ${initramfs_dir}/{bin,sbin,proc,sys,dev} || exit 1
        ${mkdir} -p ${initramfs_dir}/mnt/{root,tmp} || exit 1
        ${mkdir} -p ${initramfs_dir}/usr/{bin,sbin} || exit 1
else
	AC_MSG_ERROR([the image directory does not exist])
fi

dnl Optional feature gdb
AC_ARG_ENABLE(gdb,
	AS_HELP_STRING([--enable-gdb], [build and include gdb]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_gdb="${enableval}"], [enable_gdb="no"]), enable_gdb="no")
AS_IF([test "x$enable_gdb" = "xyes"], [build_opts="${build_opts} gdb"])
AC_SUBST(enable_gdb, ${enable_gdb})
AC_MSG_NOTICE([gdb: ${enable_gdb}])

dnl Optional feature valgrind
AC_ARG_ENABLE(valgrind,
	AS_HELP_STRING([--enable-valgrind], [build and include valgrind]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_valgrind="${enableval}"], [enable_valgrind="no"]), enable_valgrind="no")
AS_IF([test "x$enable_valgrind" = "xyes"], [build_opts="${build_opts} valgrind"])
AC_SUBST(enable_valgrind, ${enable_valgrind})
AC_MSG_NOTICE([valgrind: ${enable_valgrind}])

dnl Optional package ocs_sdk (user provides package file)
AC_ARG_WITH(ocs_sdk,
	AS_HELP_STRING([--with-ocs_sdk=PATH],
	[build and include ocs_sdk; PATH is the ocs_sdk package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[ocs_sdk_file="${withval}"; with_ocs_sdk="yes"],
	[with_ocs_sdk="no"]), with_ocs_sdk="no")
if test  "x${with_ocs_sdk}" = "xyes"; then
	if test -f "${ocs_sdk_file}"; then
		AC_MSG_NOTICE([extracting the ocs_sdk package file...])
		ocs_sdk_dir="${build_dir}/ocs_sdk"
		${mkdir} -p ${ocs_sdk_dir} || exit 1
		${tar} xfz ${ocs_sdk_file} -C ${ocs_sdk_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the ocs_sdk file: ${ocs_sdk_file}])
	fi
fi
AS_IF([test "x$with_ocs_sdk" = "xyes"], [build_opts="${build_opts} ocs_sdk"])
AC_SUBST(ocs_sdk_dir, ${ocs_sdk_dir})
AC_SUBST(with_ocs_sdk, ${with_ocs_sdk})
AC_MSG_NOTICE([ocs_sdk: ${with_ocs_sdk}])

AC_SUBST(build_opts, m4_strip($(echo ${build_opts})))

AC_OUTPUT(Makefile)
