dnl Wrapper for AC_CHECK_PROG that kills configure script if program is missing
AC_DEFUN([MY_PATH_PROG],
	[AC_CHECK_PROG([$1],[$1],[$1],NOT_FOUND,$PATH:/sbin:/usr/sbin:/usr/local/sbin)
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
MY_PATH_PROG([grep])
MY_PATH_PROG([dd])
MY_PATH_PROG([mkfs])
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
MY_PATH_PROG([flock])
MY_PATH_PROG([zic])
MY_PATH_PROG([mv])

dnl Setup build directories
if test x"${PWD}" != x; then
	AC_MSG_NOTICE([setting up build directories...])
	work_dir="${PWD}/work"
	${mkdir} -p ${work_dir} || exit 1
	AC_SUBST(work_dir, ${work_dir})
	dist_files_dir="${work_dir}/dist_files"
	${mkdir} -p ${dist_files_dir} || exit 1
	AC_SUBST(dist_files_dir, ${dist_files_dir})
	chroot_dir="${work_dir}/chroot"
	${mkdir} -p ${chroot_dir} || exit 1
	AC_SUBST(chroot_dir, ${chroot_dir})
	sources_dir="${chroot_dir}/sources"
	${mkdir} -p ${sources_dir} || exit 1
	AC_SUBST(sources_dir, ${sources_dir})
	tools_dir="${chroot_dir}/tools"
	${mkdir} -p ${tools_dir} || exit 1
	AC_SUBST(tools_dir, ${tools_dir})
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
if test -d "${chroot_dir}"; then
	AC_MSG_NOTICE([creating image directory structure...])
	${mkdir} -p ${chroot_dir}/{build,etc,bin,sbin,dev,proc,sys,root,home,run} || exit 1
	${mkdir} -p ${chroot_dir}/boot/grub || exit 1
	${mkdir} -p ${chroot_dir}/mnt/{root,conf,logs,vdisks,mhvtl} || exit 1
	${mkdir} -p ${chroot_dir}/lib/firmware || exit 1
	${mkdir} -p ${chroot_dir}/usr/{bin,sbin,libexec,lib} || exit 1
	${mkdir} -p ${chroot_dir}/usr/local/{bin,sbin} || exit 1
	${mkdir} -p ${chroot_dir}/opt/{bin,sbin,lib} || exit 1
	${mkdir} -p ${chroot_dir}/run/lvm || exit 1
	${mkdir} -p -m 1777 ${chroot_dir}/tmp || exit 1
	${mkdir} -p ${chroot_dir}/var/{spool,lock,run,state,cache,log} || exit 1
	${mkdir} -p ${chroot_dir}/var/lock/subsys || exit 1
	${mkdir} -p -m 755 ${chroot_dir}/var/empty || exit 1
	${mkdir} -p -m 1777 ${chroot_dir}/var/tmp || exit 1
	${mkdir} -p -m 710 ${chroot_dir}/var/cron || exit 1
	${mkdir} -p -m 700 ${chroot_dir}/var/cron/tabs || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/corosync || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/pacemaker/{cores,pengine,cib,blackbox} || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/pacemaker/cores/{root,hacluster} || exit 1
	${mkdir} -p ${chroot_dir}/var/run/{crm,mdadm,stunnel,web_ui} || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/heartbeat/{cores,lrm} || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/scst/{pr,vdev_mode_pages} || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/stunnel || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/lldpad || exit 1
	${mkdir} -p ${chroot_dir}/var/lib/esos || exit 1
	${ln} -sf lib ${chroot_dir}/lib64 || exit 1
	${ln} -sf lib ${chroot_dir}/usr/lib64 || exit 1
	${mkdir} -p ${tools_dir}/lib || exit 1
	${ln} -sf lib ${tools_dir}/lib64 || exit 1
	${mkdir} -p ${initramfs_dir}/{bin,sbin,proc,sys,dev,tmp} || exit 1
	${mkdir} -p ${initramfs_dir}/mnt/{root,tmp} || exit 1
	${mkdir} -p ${initramfs_dir}/usr/{bin,sbin} || exit 1
	${mkdir} -p ${initramfs_dir}/lib/firmware || exit 1
else
	AC_MSG_ERROR([the image directory does not exist])
fi

dnl Optional feature debug
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [build for debugging (do not strip userland binaries)]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_debug="${enableval}"], [enable_debug="no"]), enable_debug="no")
AS_IF([test "x$enable_debug" = "xyes"],
	[build_opts="${build_opts} debug"; opt_suffix="${opt_suffix}d"])
AC_SUBST(enable_debug, ${enable_debug})
AC_MSG_NOTICE([debug: ${enable_debug}])

dnl Optional feature gdb
AC_ARG_ENABLE(gdb,
	AS_HELP_STRING([--enable-gdb], [build and include gdb]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_gdb="${enableval}"], [enable_gdb="no"]), enable_gdb="no")
AS_IF([test "x$enable_gdb" = "xyes"],
	[build_opts="${build_opts} gdb"; opt_suffix="${opt_suffix}g"])
AC_SUBST(enable_gdb, ${enable_gdb})
AC_MSG_NOTICE([gdb: ${enable_gdb}])

dnl Optional feature valgrind
AC_ARG_ENABLE(valgrind,
	AS_HELP_STRING([--enable-valgrind], [build and include valgrind]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_valgrind="${enableval}"], [enable_valgrind="no"]), enable_valgrind="no")
AS_IF([test "x$enable_valgrind" = "xyes"],
	[build_opts="${build_opts} valgrind"; opt_suffix="${opt_suffix}v"])
AC_SUBST(enable_valgrind, ${enable_valgrind})
AC_MSG_NOTICE([valgrind: ${enable_valgrind}])

dnl Optional feature strace
AC_ARG_ENABLE(strace,
	AS_HELP_STRING([--enable-strace], [build and include strace]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_strace="${enableval}"], [enable_strace="no"]), enable_strace="no")
AS_IF([test "x$enable_strace" = "xyes"],
	[build_opts="${build_opts} strace"; opt_suffix="${opt_suffix}s"])
AC_SUBST(enable_strace, ${enable_strace})
AC_MSG_NOTICE([strace: ${enable_strace}])

dnl Optional feature qla
AC_ARG_ENABLE(qla,
	AS_HELP_STRING([--enable-qla], [use the Cavium Fibre Channel scst_qla2xxx driver instead of the SCST in-tree driver]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_qla="${enableval}"], [enable_qla="no"]), enable_qla="no")
AS_IF([test "x$enable_qla" = "xyes"],
	[build_opts="${build_opts} qla"; opt_suffix="${opt_suffix}q"])
AC_SUBST(enable_qla, ${enable_qla})
AC_MSG_NOTICE([qla: ${enable_qla}])

dnl Optional feature zfs
AC_ARG_ENABLE(zfs,
	AS_HELP_STRING([--enable-zfs], [build and include ZFS]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_zfs="${enableval}"], [enable_zfs="no"]), enable_zfs="no")
AS_IF([test "x$enable_zfs" = "xyes"],
	[build_opts="${build_opts} zfs"; opt_suffix="${opt_suffix}z"])
AC_SUBST(enable_zfs, ${enable_zfs})
AC_MSG_NOTICE([zfs: ${enable_zfs}])

dnl Optional feature mhvtl
AC_ARG_ENABLE(mhvtl,
	AS_HELP_STRING([--enable-mhvtl], [build and include mhVTL (broken)]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_mhvtl="${enableval}"], [enable_mhvtl="no"]), enable_mhvtl="no")
AS_IF([test "x$enable_mhvtl" = "xyes"],
	[build_opts="${build_opts} mhvtl"; opt_suffix="${opt_suffix}m"])
AC_SUBST(enable_mhvtl, ${enable_mhvtl})
AC_MSG_NOTICE([mhvtl: ${enable_mhvtl}])

dnl Optional feature eio
AC_ARG_ENABLE(eio,
	AS_HELP_STRING([--enable-eio], [build and include EnhanceIO (broken)]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_eio="${enableval}"], [enable_eio="no"]), enable_eio="no")
AS_IF([test "x$enable_eio" = "xyes"],
	[build_opts="${build_opts} eio"; opt_suffix="${opt_suffix}e"])
AC_SUBST(enable_eio, ${enable_eio})
AC_MSG_NOTICE([eio: ${enable_eio}])

dnl Optional feature btier
AC_ARG_ENABLE(btier,
	AS_HELP_STRING([--enable-btier], [build and include BTIER (broken)]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_btier="${enableval}"], [enable_btier="no"]), enable_btier="no")
AS_IF([test "x$enable_btier" = "xyes"],
	[build_opts="${build_opts} btier"; opt_suffix="${opt_suffix}b"])
AC_SUBST(enable_btier, ${enable_btier})
AC_MSG_NOTICE([btier: ${enable_btier}])

dnl Optional simple text-based user interface (TUI)
AC_ARG_ENABLE(simple_tui,
	AS_HELP_STRING([--enable-simple_tui], [build the simple TUI version]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_simple_tui="${enableval}"], [enable_simple_tui="no"]),
	enable_simple_tui="no")
AS_IF([test "x$enable_simple_tui" = "xyes"],
	[build_opts="${build_opts} simple_tui"; opt_suffix="${opt_suffix}S"])
AC_SUBST(enable_simple_tui, ${enable_simple_tui})
AC_MSG_NOTICE([simple_tui: ${enable_simple_tui}])

dnl Optional package web_ui (user provides package file)
AC_ARG_WITH(web_ui,
	AS_HELP_STRING([--with-web_ui=PATH],
	[build and include web_ui; PATH is the web_ui package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[web_ui_file="${withval}"; with_web_ui="yes"],
	[with_web_ui="no"]), with_web_ui="no")
if test "x${with_web_ui}" = "xyes"; then
	if test -f "${web_ui_file}"; then
		AC_MSG_NOTICE([extracting the web_ui package file...])
		web_ui_dir="${chroot_dir}/build/web_ui"
		${mkdir} -p ${web_ui_dir} || exit 1
		${tar} xfJ ${web_ui_file} --strip-components=1 -C ${web_ui_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the web_ui file: ${web_ui_file}])
	fi
fi
AS_IF([test "x$with_web_ui" = "xyes"],
	[build_opts="${build_opts} web_ui"; opt_suffix="${opt_suffix}w"])
AC_SUBST(web_ui_dir, ${web_ui_dir})
AC_SUBST(with_web_ui, ${with_web_ui})
AC_MSG_NOTICE([web_ui: ${with_web_ui}])

dnl Optional package rpc_agent (user provides package file)
AC_ARG_WITH(rpc_agent,
	AS_HELP_STRING([--with-rpc_agent=PATH],
	[build and include rpc_agent; PATH is the rpc_agent package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[rpc_agent_file="${withval}"; with_rpc_agent="yes"],
	[with_rpc_agent="no"]), with_rpc_agent="no")
if test "x${with_rpc_agent}" = "xyes"; then
	if test -f "${rpc_agent_file}"; then
		AC_MSG_NOTICE([extracting the rpc_agent package file...])
		rpc_agent_dir="${chroot_dir}/build/rpc_agent"
		${mkdir} -p ${rpc_agent_dir} || exit 1
		${tar} xfJ ${rpc_agent_file} --strip-components=1 -C ${rpc_agent_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the rpc_agent file: ${rpc_agent_file}])
	fi
fi
AS_IF([test "x$with_rpc_agent" = "xyes"],
	[build_opts="${build_opts} rpc_agent"; opt_suffix="${opt_suffix}r"])
AC_SUBST(rpc_agent_dir, ${rpc_agent_dir})
AC_SUBST(with_rpc_agent, ${with_rpc_agent})
AC_MSG_NOTICE([rpc_agent: ${with_rpc_agent}])

dnl Optional commerical edition of ESOS (Apache 2.0 is default)
AC_ARG_ENABLE(commercial,
	AS_HELP_STRING([--enable-commercial], [build the commercial edition]),
	AS_IF([test "x${enableval}" = "xyes" || test "x${enableval}" = "xno"],
	[enable_commercial="${enableval}"], [enable_commercial="no"]),
	enable_commercial="no")
if test "x${enable_commercial}" = "xyes"; then
	if test "x${with_web_ui}" != "xyes"; then
		AC_MSG_ERROR([the commercial edition requires the web_ui build option])
	fi
	if test "x${with_rpc_agent}" != "xyes"; then
		AC_MSG_ERROR([the commercial edition requires the rpc_agent build option])
	fi
fi
AS_IF([test "x$enable_commercial" = "xyes"],
	[build_opts="${build_opts} commercial"; opt_suffix="${opt_suffix}C"])
AC_SUBST(enable_commercial, ${enable_commercial})
AC_MSG_NOTICE([commercial: ${enable_commercial}])

dnl Optional package celerity_16_32 (user provides package file)
AC_ARG_WITH(celerity_16_32,
	AS_HELP_STRING([--with-celerity_16_32=PATH],
	[build and include celerity_16_32; PATH is the celerity_16_32 package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[celerity_16_32_file="${withval}"; with_celerity_16_32="yes"],
	[with_celerity_16_32="no"]), with_celerity_16_32="no")
if test "x${with_celerity_16_32}" = "xyes"; then
	if test -f "${celerity_16_32_file}"; then
		AC_MSG_NOTICE([extracting the celerity_16_32 package file...])
		celerity_16_32_dir="${chroot_dir}/build/celerity_16_32"
		${mkdir} -p ${celerity_16_32_dir} || exit 1
		${tar} xfz ${celerity_16_32_file} --strip-components=1 -C ${celerity_16_32_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the celerity_16_32 file: ${celerity_16_32_file}])
	fi
fi
AS_IF([test "x$with_celerity_16_32" = "xyes"],
	[build_opts="${build_opts} celerity_16_32"; opt_suffix="${opt_suffix}c"])
AC_SUBST(celerity_16_32_dir, ${celerity_16_32_dir})
AC_SUBST(with_celerity_16_32, ${with_celerity_16_32})
AC_MSG_NOTICE([celerity_16_32: ${with_celerity_16_32}])

dnl Optional package atto_scst (user provides package file)
AC_ARG_WITH(atto_scst,
	AS_HELP_STRING([--with-atto_scst=PATH],
	[build and include atto_scst; PATH is the atto_scst package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[atto_scst_file="${withval}"; with_atto_scst="yes"],
	[with_atto_scst="no"]), with_atto_scst="no")
if test "x${with_atto_scst}" = "xyes"; then
	if test "x${with_celerity_16_32}" != "xyes"; then
		AC_MSG_ERROR([the ATTO SCST driver requires the Celerity 16/32 build option])
	fi
	if test -f "${atto_scst_file}"; then
		AC_MSG_NOTICE([extracting the atto_scst package file...])
		atto_scst_dir="${chroot_dir}/build/atto_scst"
		${mkdir} -p ${atto_scst_dir} || exit 1
		${tar} xfz ${atto_scst_file} --strip-components=1 -C ${atto_scst_dir} || exit 1
		${find} ${atto_scst_dir} -type f -exec ${chmod} 0644 {} \; || exit 1
	else
		AC_MSG_ERROR([unable to access the atto_scst file: ${atto_scst_file}])
	fi
fi
AS_IF([test "x$with_atto_scst" = "xyes"],
	[build_opts="${build_opts} atto_scst"; opt_suffix="${opt_suffix}a"])
AC_SUBST(atto_scst_dir, ${atto_scst_dir})
AC_SUBST(with_atto_scst, ${with_atto_scst})
AC_MSG_NOTICE([atto_scst: ${with_atto_scst}])

dnl Optional package ocs_sdk (user provides package file)
AC_ARG_WITH(ocs_sdk,
	AS_HELP_STRING([--with-ocs_sdk=PATH],
	[build and include ocs_sdk; PATH is the ocs_sdk package file to use]),
	AS_IF([test "x${withval}" != "xyes" && test "x${withval}" != "xno"],
	[ocs_sdk_file="${withval}"; with_ocs_sdk="yes"],
	[with_ocs_sdk="no"]), with_ocs_sdk="no")
if test "x${with_ocs_sdk}" = "xyes"; then
	if test -f "${ocs_sdk_file}"; then
		AC_MSG_NOTICE([extracting the ocs_sdk package file...])
		ocs_sdk_dir="${chroot_dir}/build/ocs_sdk"
		${mkdir} -p ${ocs_sdk_dir} || exit 1
		${tar} xfz ${ocs_sdk_file} -C ${ocs_sdk_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the ocs_sdk file: ${ocs_sdk_file}])
	fi
fi
AS_IF([test "x$with_ocs_sdk" = "xyes"],
	[build_opts="${build_opts} ocs_sdk"; opt_suffix="${opt_suffix}o"])
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
if test "x${with_uwire}" = "xyes"; then
	AC_MSG_ERROR([support for Chelsio Unified Wire adapters is broken])
	if test -f "${uwire_file}"; then
		AC_MSG_NOTICE([extracting the uwire package file...])
		uwire_dir="${chroot_dir}/build/uwire"
		${mkdir} -p ${uwire_dir} || exit 1
		${tar} xfz ${uwire_file} --strip-components=1 -C ${uwire_dir} || exit 1
	else
		AC_MSG_ERROR([unable to access the uwire file: ${uwire_file}])
	fi
fi
AS_IF([test "x$with_uwire" = "xyes"],
	[build_opts="${build_opts} uwire"; opt_suffix="${opt_suffix}u"])
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

AS_IF([test "x${opt_suffix}" != "x"], [opt_suffix="_${opt_suffix}"])
AC_SUBST(build_opts, m4_strip($(echo ${build_opts})))
AC_SUBST(opt_suffix, m4_strip($(echo ${opt_suffix})))

AC_OUTPUT(Makefile)
