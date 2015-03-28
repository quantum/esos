### Introduction ###
ESOS started with an internal need for a fully functioning SCST / Linux system that could easily be deployed to new servers. We also wanted this Linux "distribution" to be fully optimized for SCST and include necessary RAID controller tools/utilities so new volumes could easily be provisioned/modified from inside the OS.

After some time passed and our comfort level with ESOS/SCST grew, we realized the next logical step: Highly available storage clusters. We pumped a number of new features (software projects) into ESOS, including, but not limited to DRBD, LVM2, Linux software RAID (md), mhVTL, and a full-featured cluster stack (Pacemaker + Corosync).

<br>

<h3>Included Projects</h3>
ESOS uses the software projects listed below; you can check the <a href='https://code.google.com/p/enterprise-storage-os/source/browse/trunk/Makefile.in'>Makefile</a> (trunk) for the specific versions (varies by release).<br>
<ul><li>Linux kernel<br>
</li><li>SCST<br>
</li><li>BusyBox<br>
</li><li>GRUB<br>
</li><li>SysVinit<br>
</li><li>GLIBC<br>
</li><li>vixie-cron<br>
</li><li>libibumad<br>
</li><li>libibverbs<br>
</li><li>srptools<br>
</li><li>OpenSSH<br>
</li><li>sSMTP<br>
</li><li>Perl<br>
</li><li>OpenSSL<br>
</li><li>e2fsprogs<br>
</li><li>zlib<br>
</li><li>lsscsi<br>
</li><li>sg3_utils<br>
</li><li>groff<br>
</li><li>ncurses<br>
</li><li>kexec-tools<br>
</li><li>GCC (only libstdc++ and libgcc are installed to image)<br>
</li><li>iniparser<br>
</li><li>CDK<br>
</li><li>DRBD<br>
</li><li>LVM2<br>
</li><li>QLogic Fibre Channel Binary Firmware<br>
</li><li>xfsprogs<br>
</li><li>mdadm<br>
</li><li>GNU Parted<br>
</li><li>libqb<br>
</li><li>Pacemaker<br>
</li><li>Corosync<br>
</li><li>nss<br>
</li><li>glib<br>
</li><li>libxml2<br>
</li><li>libxslt<br>
</li><li>libtool<br>
</li><li>bzip2<br>
</li><li>Python<br>
</li><li>crmsh<br>
</li><li>libaio<br>
</li><li>glue<br>
</li><li>readline<br>
</li><li>resource-agents<br>
</li><li>mhVTL<br>
</li><li>lzo<br>
</li><li>mhash<br>
</li><li>lessfs<br>
</li><li>tokyocabinet<br>
</li><li>FUSE<br>
</li><li>Berkeley DB<br>
</li><li>Google's Snappy<br>
</li><li>fence-agents<br>
</li><li>OpenSM<br>
</li><li>pycurl<br>
</li><li>curl<br>
</li><li>Net-Telnet (Perl module)<br>
</li><li>python-suds (Python module)<br>
</li><li>setuptools (Python module)<br>
</li><li>pexpect (Python module)<br>
</li><li>GNU Bash<br>
</li><li>Open-FCoE<br>
</li><li>Open-LLDP<br>
</li><li>libconfig<br>
</li><li>libnl<br>
</li><li>libpciaccess<br>
</li><li>Linux Firmware (package of binary blobs)<br>
</li><li>dlm<br>
</li><li>sysklogd<br>
</li><li>ipmitool<br>
</li><li>less<br>
</li><li>fio</li></ul>

Several other proprietary pieces are options that can be downloaded and included at install time:<br>
<ul><li>MegaCli64 (for LSI Logic MegaRAID controllers)<br>
</li><li>arcconf (for Adaptec AACRAID controllers)<br>
</li><li>hpacucli (for HP Smart Array controllers)<br>
</li><li>cli64 (for Areca RAID controllers)<br>
</li><li>tw_cli.x86_64 (for 3ware SATA/SAS RAID controllers)</li></ul>

<br>

<h3>How It Works</h3>
ESOS boots from a USB flash drive; all of the binaries/files/directories/etc. are loaded into memory on boot. If the USB flash drive fails, the system will keep running normally until the failed flash drive can be addressed (replaced). Configuration files and settings are sync'd to a file system since ESOS is volatile (memory resident). Log files are also archived to the USB drive on shutdown/restart or if the file system grows too large. This also provides an easy and reversible upgrade procedure: You simply create a new, updated ESOS USB flash drive, copy your configuration to it, and boot the new drive -- if you happen to experience an issue with the new version, you can always boot back into your previous ESOS USB drive.<br>
<br>
Here is a high level step-through of the ESOS boot process:<br>
<ol><li>The ESOS USB flash drive is used as the BIOS boot device.<br>
</li><li>GRUB is loaded; user can select between ESOS 'Production' or 'Debug' kernel/modules.<br>
</li><li>Selected kernel (and initramfs image) is loaded; initramfs init then takes care of various prep. tasks, initializes tmpfs file system (RAM) and then extracts root image into newly created tmpfs file system.<br>
</li><li>Control is then passed to init and the rc/init scripts are executed.<br>
</li><li>Running ESOS configuration is synchronized with USB flash drive.<br>
</li><li>Various daemons (sshd, crond, etc.) are started, HBA/HCA/CNA and SCST modules are loaded.</li></ol>

In the hopefully rare case of kernel panics, kexec is implemented in ESOS. At boot, a crash dump kernel is loaded. If/when the kernel panics, the system loads the crash dump kernel. The initramfs init script is ran again and it looks for the '/proc/vmcore' file indicating the crash dump kernel is running, due to a kernel panic. The vmcore file is then compressed and saved onto the "esos_logs" filesystem. Finally the system does a full reboot and boots back into the normal/production ESOS kernel. The start-up scripts look for any saved vmcore files and will email an alert. This is all fully automated -- the idea is to save the kernel panic information for diagnosing at a later time and get your ESOS storage server back into production mode as quickly as possible.