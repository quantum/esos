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
MY_PATH_PROG([zip])
MY_PATH_PROG([xz])
MY_PATH_PROG([readlink])
MY_PATH_PROG([strip])
MY_PATH_PROG([bzip2])
MY_PATH_PROG([git])
MY_PATH_PROG([cut])

dnl Setup build directories
if test x"${PWD}" != x; then
	AC_MSG_NOTICE([setting up build directories...])
	work_dir="${PWD}/work"
	${mkdir} -p ${work_dir} || exit 1
	AC_SUBST(work_dir, ${work_dir})
	dist_files_dir="${work_dir}/dist_files"
	${mkdir} -p ${dist_files_dir} || exit 1
	AC_SUBST(dist_files_dir, ${dist_files_dir})
	pkg_src_dir="${work_dir}/build"
	${mkdir} -p ${pkg_src_dir} || exit 1
	AC_SUBST(pkg_src_dir, ${pkg_src_dir})
	image_dir="${work_dir}/image"
	${mkdir} -p ${image_dir} || exit 1
	AC_SUBST(image_dir, ${image_dir})
	initramfs_dir="${work_dir}/initramfs"
	${mkdir} -p ${initramfs_dir} || exit 1
	AC_SUBST(initramfs_dir, ${initramfs_dir})
	mount_dir="${work_dir}/mnt"
	${mkdir} -p ${mount_dir} || exit 1
	AC_SUBST(mount_dir, ${mount_dir})
	staging_dir="${work_dir}/staging"
	${mkdir} -p ${staging_dir} || exit 1
	AC_SUBST(staging_dir, ${staging_dir})
else
	AC_MSG_ERROR([invalid current working directory])
fi

dnl Create directory structure for the image
if test -d "${image_dir}"; then
	AC_MSG_NOTICE([creating image directory structure...])
	${mkdir} -p ${image_dir}/{etc,bin,sbin,dev,proc,sys,root,home,run} || exit 1
	${mkdir} -p ${image_dir}/boot/grub || exit 1
	${mkdir} -p ${image_dir}/mnt/{root,conf,logs,vdisks,mhvtl} || exit 1
	${mkdir} -p ${image_dir}/lib/firmware || exit 1
	${mkdir} -p ${image_dir}/usr/{bin,sbin,libexec,lib} || exit 1
	${mkdir} -p ${image_dir}/usr/local/{bin,sbin} || exit 1
	${mkdir} -p ${image_dir}/opt/{bin,sbin,lib} || exit 1
	${mkdir} -p ${image_dir}/run/lvm || exit 1
	${mkdir} -p -m 1777 ${image_dir}/tmp || exit 1
	${mkdir} -p ${image_dir}/var/{spool,lock,run,state,cache,log} || exit 1
	${mkdir} -p ${image_dir}/var/lock/subsys || exit 1
	${mkdir} -p -m 755 ${image_dir}/var/empty || exit 1
	${mkdir} -p -m 1777 ${image_dir}/var/tmp || exit 1
	${mkdir} -p -m 710 ${image_dir}/var/cron || exit 1
	${mkdir} -p -m 700 ${image_dir}/var/cron/tabs || exit 1
	${mkdir} -p ${image_dir}/var/lib/corosync || exit 1
	${mkdir} -p ${image_dir}/var/lib/pacemaker/{cores,pengine,cib,blackbox} || exit 1
	${mkdir} -p ${image_dir}/var/lib/pacemaker/cores/{root,hacluster} || exit 1
	${mkdir} -p ${image_dir}/var/run/{crm,mdadm,stunnel} || exit 1
	${mkdir} -p ${image_dir}/var/lib/heartbeat/{cores,lrm} || exit 1
	${mkdir} -p ${image_dir}/var/lib/scst/{pr,vdev_mode_pages} || exit 1
	${mkdir} -p ${image_dir}/var/lib/stunnel || exit 1
	${mkdir} -p ${image_dir}/var/lib/lldpad || exit 1
	${ln} -sf lib ${image_dir}/lib64 || exit 1
	${ln} -sf lib ${image_dir}/usr/lib64 || exit 1
	${mkdir} -p ${initramfs_dir}/{bin,sbin,proc,sys,dev,tmp} || exit 1
	${mkdir} -p ${initramfs_dir}/mnt/{root,tmp} || exit 1
	${mkdir} -p ${initramfs_dir}/usr/{bin,sbin} || exit 1
else
	AC_MSG_ERROR([the image directory does not exist])
fi

dnl Optional feature debug
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [build for debugging (do not strip userland binaries)]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_debug="${enableval}"], [enable_debug="no"]), enable_debug="no")
AS_IF([test "x$enable_debug" = "xyes"], [build_opts="${build_opts} debug"])
AC_SUBST(enable_debug, ${enable_debug})
AC_MSG_NOTICE([debug: ${enable_debug}])

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

dnl Optional feature strace
AC_ARG_ENABLE(strace,
	AS_HELP_STRING([--enable-strace], [build and include strace]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_strace="${enableval}"], [enable_strace="no"]), enable_strace="no")
AS_IF([test "x$enable_strace" = "xyes"], [build_opts="${build_opts} strace"])
AC_SUBST(enable_strace, ${enable_strace})
AC_MSG_NOTICE([strace: ${enable_strace}])

dnl Optional feature qla
AC_ARG_ENABLE(qla,
	AS_HELP_STRING([--enable-qla], [use the QLogic provided qla2xxx driver instead of the SCST in-tree driver]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_qla="${enableval}"], [enable_qla="no"]), enable_qla="no")
AS_IF([test "x$enable_qla" = "xyes"], [build_opts="${build_opts} qla"])
AC_SUBST(enable_qla, ${enable_qla})
AC_MSG_NOTICE([qla: ${enable_qla}])

dnl Optional feature zfs
AC_ARG_ENABLE(zfs,
	AS_HELP_STRING([--enable-zfs], [build and include ZFS]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_zfs="${enableval}"], [enable_zfs="no"]), enable_zfs="no")
AS_IF([test "x$enable_zfs" = "xyes"], [build_opts="${build_opts} zfs"])
AC_SUBST(enable_zfs, ${enable_zfs})
AC_MSG_NOTICE([zfs: ${enable_zfs}])

dnl Optional feature eio
AC_ARG_ENABLE(eio,
	AS_HELP_STRING([--enable-eio], [build and include EnhanceIO]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_eio="${enableval}"], [enable_eio="no"]), enable_eio="no")
AS_IF([test "x$enable_eio" = "xyes"], [build_opts="${build_opts} eio"])
AC_SUBST(enable_eio, ${enable_eio})
AC_MSG_NOTICE([eio: ${enable_eio}])

dnl Optional feature btier
AC_ARG_ENABLE(btier,
	AS_HELP_STRING([--enable-btier], [build and include BTIER]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_btier="${enableval}"], [enable_btier="no"]), enable_btier="no")
AS_IF([test "x$enable_btier" = "xyes"], [build_opts="${build_opts} btier"])
AC_SUBST(enable_btier, ${enable_btier})
AC_MSG_NOTICE([btier: ${enable_btier}])

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
		ocs_sdk_dir="${work_dir}/ocs_sdk"
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

dnl Optional package uwire (user provides package file)
AC_ARG_WITH(uwire,
	AS_HELP_STRING([--with-uwire=PATH],
	[build and include uwire; PATH is the uwire package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[uwire_file="${withval}"; with_uwire="yes"],
	[with_uwire="no"]), with_uwire="no")
if test  "x${with_uwire}" = "xyes"; then
	AC_MSG_ERROR([support for Chelsio Unified Wire adapters is broken])
	if test -f "${uwire_file}"; then
		AC_MSG_NOTICE([extracting the uwire package file...])
		uwire_dir="${work_dir}/uwire"
		${mkdir} -p ${uwire_dir} || exit 1
		${tar} xfz ${uwire_file} --strip-components=1 -C ${uwire_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the uwire file: ${uwire_file}])
	fi
fi
AS_IF([test "x$with_uwire" = "xyes"], [build_opts="${build_opts} uwire"])
AC_SUBST(uwire_dir, ${uwire_dir})
AC_SUBST(with_uwire, ${with_uwire})
AC_MSG_NOTICE([uwire: ${with_uwire}])

AS_IF([test "x${srcdir}" = "x."],
	[real_src_dir="${PWD}"],
	[real_src_dir="${srcdir}"])
AC_SUBST(src_dir, ${real_src_dir})
AC_MSG_NOTICE([src_dir: ${real_src_dir}])
AC_SUBST(build_dir, ${PWD})
AC_MSG_NOTICE([build_dir: ${PWD}])

AC_SUBST(build_opts, m4_strip($(echo ${build_opts})))

AC_OUTPUT(Makefile)
