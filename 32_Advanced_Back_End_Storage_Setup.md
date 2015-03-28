### Distributed Replicated Block Device (DRBD) ###
DRBD makes an excellent partner to SCST for creating highly available disk arrays and/or replicating data to a remote site for business continuity or disaster recovery purposes. ESOS includes a mostly vanilla DRBD install with the full set of user-land tools and DRBD functionality built-in to the kernel; we are currently using the 8.4 version of DRBD. The DRBD [documentation](http://www.drbd.org/users-guide-8.4/) is quite excellent, so we won't even try to emulate that here. _Use the official 8.4 DRBD documentation._

Since DRBD resources (starting/stopping) are typically handled by the cluster stack (Pacemaker + Corosync), the stand-alone DRBD service is disabled by default. To setup/configure the DRBD resources on boot, edit the /etc/rc.conf file and set `rc.drbd_enable` to "YES".

We'll provide a brief example DRBD setup of (2) nodes with one dual-primary resource. Here is the /etc/drbd.d/global\_common.conf configuration file we used on both ESOS nodes:
```
global {
        usage-count no;
}
common {
        handlers {
                pri-on-incon-degr "/usr/lib/drbd/notify-pri-on-incon-degr.sh; /usr/lib/drbd/notify-emergency-reboot.sh; echo b > /proc/sysrq-trigger ; reboot -f";
                pri-lost-after-sb "/usr/lib/drbd/notify-pri-lost-after-sb.sh; /usr/lib/drbd/notify-emergency-reboot.sh; echo b > /proc/sysrq-trigger ; reboot -f";
                local-io-error "/usr/lib/drbd/notify-io-error.sh; /usr/lib/drbd/notify-emergency-shutdown.sh; echo o > /proc/sysrq-trigger ; halt -f";
                fence-peer "/usr/lib/drbd/crm-fence-peer.sh";
                split-brain "/usr/lib/drbd/notify-split-brain.sh root";
                out-of-sync "/usr/lib/drbd/notify-out-of-sync.sh root";
                after-resync-target "/usr/lib/drbd/crm-unfence-peer.sh";
        }
        startup {
                degr-wfc-timeout 120;
                outdated-wfc-timeout 2;
        }
        options {
                on-no-data-accessible io-error;
        }
        disk {
                on-io-error detach;
                disk-barrier no;
                disk-flushes no;
                fencing resource-only;
                al-extents 3389;
        }
        net {
                protocol C;
                after-sb-0pri discard-zero-changes;
                after-sb-1pri discard-secondary;
                after-sb-2pri disconnect;
                rr-conflict disconnect;
                max-buffers 8000;
                max-epoch-size 8000;
                sndbuf-size 512k;
        }
}
```

Here is the resource file (/etc/drbd.d/[r0](https://code.google.com/p/enterprise-storage-os/source/detail?r=0).res) used on each host:
```
resource r0 {
        net {
                allow-two-primaries;
        }
        on bill.mcc.edu {
                device     /dev/drbd0;
                disk       /dev/sda;
                address    172.16.0.22:7788;
                meta-disk  internal;
        }
        on ben.mcc.edu {
                device    /dev/drbd0;
                disk      /dev/sda;
                address   172.16.0.21:7788;
                meta-disk internal;
        }
}
```

Next, run the following on both hosts:
```
drbdadm create-md r0
drbdadm attach r0
drbdadm syncer r0
drbdadm connect r0
```

Now run the following on only one of the hosts (assuming empty disk):
```
drbdadm -- --overwrite-data-of-peer primary r0
```

The DRBD resource will now be synchronized to the other host; you can check the status with "cat /proc/drbd" (or check the status in the TUI: Back-End Storage -> DRBD Status).

You can now make the "secondary" host a primary:
```
drbdadm primary r0
```

You should now have a dual-primary DRBD resource available as /dev/drbd0 on both nodes. You can use this block device node as an SCST device, or use an additional storage management/provisioning layer on top of the DRBD resource (LVM2, software RAID, etc.), or whatever other advanced storage configuration you might dream up.

<br>

<h3>Linux Software RAID (md)</h3>
The <code>mdadm</code> tool is provided with ESOS to manage Linux software RAID (md) arrays. Since there are so many good guides, articles, and information on using mdadm available on the Internet, we won't even mention a specific link in this document; simply Google "mdadm howto" and you'll get a whole slew of them. We'll provide a couple basic examples below. Possible ESOS storage configuration ideas that make use of software RAID might include using RAID0 across two different hardware RAID controllers to possibly gain performance, or using software RAID1 across multiple hardware RAID controllers or SCSI disks to increase reliability.<br>
<br>
Create a partition on each SCSI disk block device you'd like to use for software RAID ("Linux RAID Autodetect", type 'fd'). You can use either <code>fdisk</code> or <code>parted</code> to create partitions (both are included with ESOS).<br>
<br>
To create a RAID0 (striped) volume with a 64 KiB chunk size on two disks, run the following command:<br>
<pre><code>mdadm --create /dev/md0 --chunk=64 --level=0 --raid-devices=2 /dev/sda1 /dev/sdb1<br>
</code></pre>

To create a RAID1 (mirrored) volume with a 64 KiB chunk size on two disks, run the following command:<br>
<pre><code>mdadm --create /dev/md0 --chunk=64 --level=1 --raid-devices=2 /dev/sda1 /dev/sdb1<br>
</code></pre>

You can now use these new software RAID block devices as backing for a file system, or directly as a vdisk_blockio SCST device, or in conjunction with another storage management layer (eg, LVM2). The possibilities are limitless!<br>
<br>
<br>

<h3>Logical Volume Manager (LVM2)</h3>
The Logical Volume Manager in Linux is a time-tested, stable piece of software. Its used by default on installs in big-name Linux distributions and has proven its usefulness. In ESOS, LVM a very helpful layer when use between the back-end storage devices, and SCST targets. It allows one to partition and manage back-storage easily, and includes advanced features like snapshots. There is lots of great information on using LVM2 on the web, so we definitely won't try to cover everything here. We'll provide a few brief examples below.<br>
<br>
The clvmd daemon is also included with ESOS. This service is utilized when using LVM2 in a cluster; it prevents concurrent metadata updates from different nodes on shared storage (eg, using DRBD). The clvmd service is disabled by default. The clvmd daemon uses DLM for locking and requires a working Corosync cluster. To enable clvmd and dlm_controld, edit /etc/rc.conf, then change the value for <code>rc.dlm_enable</code> and <code>rc.clvmd_enable</code> to 'YES'. After you are sure Corosync is running and you have quorum (check it with: <code>corosync-cfgtool -s</code>) start the services:<br>
<br>
<pre><code>/etc/rc.d/rc.dlm start<br>
/etc/rc.d/rc.clvmd start<br>
</code></pre>

To create a physical volume (PV) for LVM using an entire SCSI disk (/dev/sdc):<br>
<pre><code>pvcreate -v /dev/sdc<br>
</code></pre>

Next you can create a volume group (VG) for LVM using the PV we created above:<br>
<pre><code>vgcreate -v big_space_1 /dev/sdc<br>
</code></pre>

Now we can create a 500 GB logical volume (LV) called "small_vmfs_1" that can be used with SCST:<br>
<pre><code>lvcreate -v -L 500G -n small_vmfs_1 big_space_1<br>
</code></pre>

These were a couple very basic examples that should show you how to get started with LVM2; this software is very powerful and flexible and can be used in a number of different ways. Read up on the LVM2 documentation (and man pages)!<br>
<br>
<br>

<h3>Local RAID Controller: LSI MegaRAID (CLI)</h3>
The LSI Logic "MegaCLI" utility is an install option with ESOS (during the installation script). The utility allows you to create/delete/modify volumes (logical drives) on your MegaRAID controller. Below are a few examples of using the MegaCLI tool (MegaCli64 for us). A nice handy cheat sheet is located <a href='http://tools.rapidsoft.de/perc/perc-cheat-sheet.html'>here</a>. Or for a very in-depth document, consult the user guide available on LSI Logic's web site.<br>
<br>
Get adapter information for the first MegaRAID adapter (0):<br>
<pre><code>MegaCli64 -AdpAllInfo -a0<br>
</code></pre>

A list of all the adapter's physical drives:<br>
<pre><code>MegaCli64 -PDList -a0<br>
</code></pre>

All of the logical drives for the adapter:<br>
<pre><code>MegaCli64 -LDInfo -Lall -a0<br>
</code></pre>

Delete logical drive 0:<br>
<pre><code>MegaCli64 -CfgLdDel -L0 -a0<br>
</code></pre>

Create a new RAID5 logical drive, with three physical disks, adaptive read-ahead (ADRA) and write cache enabled (WB):<br>
<pre><code>MegaCli64 -CfgLDAdd -R5[8:0,8:1,8:2] WB ADRA -a0<br>
</code></pre>

Create a new RAID0 logical drive, with two physical disks, no read-ahead (NORA) and write cache disabled (WT):<br>
<pre><code>MegaCli64 -CfgLDAdd -R0[8:0,8:1] WT NORA -a0<br>
</code></pre>

After you have created your new volume(s) you will need to find and record the SCSI device node (eg, /dev/sdz). You can easily find this using <code>lsscsi</code> or checking <code>dmesg</code>.<br>
<br>
<br>

<h3>Local RAID Controller: Adaptec AACRAID (CLI)</h3>
The arcconf utility is also an install option with ESOS for configuring Adaptec RAID controllers from inside the OS. This tool should work with most/all Adaptec SATA/SAS RAID controllers.<br>
<br>
See all controller/drive/volume information for controller # 1:<br>
<pre><code>arcconf GETCONFIG 1<br>
</code></pre>

Delete logical device (volume) 1 on controller 1:<br>
<pre><code>arcconf DELETE 1 LOGICALDRIVE 1<br>
</code></pre>

Make a new RAID5 volume on controller 1 using three disks (channel 0, device numbers 2, 3, 4) with read cache enabled and write cache enabled:<br>
<pre><code>arcconf CREATE 1 LOGICALDRIVE Rcache RON Wcache WB MAX 5 0 2 0 3 0 4<br>
</code></pre>

Make a new RAID0 volume on controller 1 using two disks (channel 0, device numbers 2, 3) with read cache disabled and write cache disabled:<br>
<pre><code>arcconf CREATE 1 LOGICALDRIVE Rcache ROFF Wcache WT MAX 0 0 2 0 3<br>
</code></pre>

Once you've created a new volume on your Adaptec RAID controller, grab the SCSI device node (<code>lsscsi</code> works well) and continue with one of the target configuration sections below.<br>
<br>
<br>

<h3>Local RAID Controller: Other</h3>
Other RAID controllers are supported, however, not all of them necessarily have a CLI tool for configuring volumes, adapter settings, etc. from inside of ESOS. See the <a href='03_Supported_Hardware.md'>03_Supported_Hardware</a> wiki page for a current list of supported controllers and possible corresponding CLI utilities.<br>
<br>
You can still use these other controllers with ESOS, you will just need to configure your volumes / logical drives "outside" of ESOS -- via the BIOS. Or seek the documentation for using the CLI tools on your own.<br>
<br>
If you find that your favorite "enterprise class" RAID controller is not supported by ESOS, please let us know on the esos-users Google Group. It would also be helpful to know if there are any CLI management tools that can be used to configure these controllers from inside the OS.<br>
<br>
<br>

<h3>Virtual Tape Library (VTL)</h3>
ESOS includes the mhVTL software (virtual tape library); this, combined with data de-duplication makes an excellent traditional tape library (eg, DLT, LTO, etc.) replacement. You can use this virtual tape library on your Storage Area Network (SAN -> Fibre Channel, iSCSI, etc.). The mhVTL service is disabled by default -- to enable it, simply edit the /etc/rc.conf file and set <code>rc.mhvtl_enable</code> to "YES".<br>
<br>
The data storage location (mount point in the file system) is hard coded in the software (/mnt/mhvtl). You'll need to create a new back-end storage device and file system then mount it on <code>/mnt/mhvtl</code> and update the fstab file.<br>
<br>
If you are going to use lessfs for de-duplication as the mhVTL storage backing, you will want to use a separate persistent storage device (RAID volume) for the lessfs configuration data and database. You will then use the <code>/mnt/mhvtl</code> location for a separate underlying back-end storage file system, and then use the same location as the lessfs mount point. The key point here is that you need two separate persistent file systems for this setup with lessfs and mhVTL: One for the lessfs data, and one the mhVTL data.<br>
<br>
We'll present a very basic mhVTL configuration on this page. See the mhVTL <a href='https://sites.google.com/site/linuxvtl2/'>page</a> for additional setup/configuration information.<br>
<br>
First, create the /etc/mhvtl/device.conf file:<br>
<pre><code>VERSION: 3<br>
<br>
Library: 10 CHANNEL: 0 TARGET: 1 LUN: 0<br>
 Vendor identification: SPECTRA<br>
 Product identification: PYTHON<br>
 Product revision level: 5500<br>
 Unit serial number: XYZZY_10<br>
 NAA: 10:22:33:44:ab:cd:ef:00<br>
<br>
Drive: 11 CHANNEL: 0 TARGET: 1 LUN: 1<br>
 Library ID: 10 Slot: 1<br>
 Vendor identification: QUANTUM<br>
 Product identification: SDLT600<br>
 Product revision level: 5500<br>
 Unit serial number: XYZZY_11<br>
 NAA: 10:22:33:44:ab:cd:ef:01<br>
 VPD: b0 04 00 02 01 00<br>
<br>
Drive: 12 CHANNEL: 0 TARGET: 1 LUN: 2<br>
 Library ID: 10 Slot: 2<br>
 Vendor identification: QUANTUM<br>
 Product identification: SDLT600<br>
 Product revision level: 5500<br>
 Unit serial number: XYZZY_12<br>
 NAA: 10:22:33:44:ab:cd:ef:02<br>
 VPD: b0 04 00 02 01 00<br>
</code></pre>

Next, create the /etc/mhvtl/library_contents.10 file:<br>
<pre><code>VERSION: 2<br>
<br>
Drive 1:<br>
Drive 2:<br>
<br>
Picker 1:<br>
<br>
MAP 1:<br>
MAP 2:<br>
MAP 3:<br>
MAP 4:<br>
<br>
Slot 01: L10001S3<br>
Slot 02: L10002S3<br>
Slot 03: L10003S3<br>
Slot 04: L10004S3<br>
Slot 05: L10005S3<br>
Slot 06: L10006S3<br>
Slot 07: L10007S3<br>
Slot 08: L10008S3<br>
Slot 09: L10009S3<br>
Slot 10: L10010S3<br>
</code></pre>

You can now start the mhVTL service in a ESOS shell:<br>
<pre><code>/etc/rc.d/rc.mhvtl start<br>
</code></pre>

You should now have a mhVTL virtual tape library running on your ESOS storage server! You can check that the robot/drives are available with the <code>lsscsi -g</code> command. After the VTL is configured, you'll use the <a href='51_Device_Configuration.md'>51_Device_Configuration</a> wiki page to create the corresponding SCST devices.<br>
<br>
<br>

<h3>Inline Data De-duplication</h3>
De-duplication in ESOS is handled by lessfs, a virtual file system for FUSE. The lessfs file system is mounted on top of a "normal" (eg, ext3, xfs, etc.) and provides compression and encryption. You can then use this lessfs file system as your back-end storage for mhVTL or SCST FILEIO devices, seamlessly providing de-duplication.<br>
<br>
A separate, unique lessfs configuration file is needed for each lessfs file system. One configuration file can not handle multiple lessfs instances. A database (local files, Berkeley DB) is used for each lessfs file system, which is configured using the lessfs configuration file.<br>
<br>
The location for the lessfs database files and configuration file needs to be a persistent attached storage device on ESOS (eg, logical drive on local RAID controller). <b>Do not use any locations on the esos_root file system (/) for storing lessfs configuration files, or databases!</b>

A typical setup for lessfs looks like this:<br>
<ul><li>Create a new back-end storage file system using the TUI. Example mount point: /mnt/vdisks/test_fs_1<br>
</li><li>You would then create your lessfs configuration file here: /mnt/vdisks/test_fs_1/lessfs.cfg<br>
</li><li>Create these three directories: /mnt/vdisks/test_fs_1/mta /mnt/vdisks/test_fs_1/dta /mnt/vdisks/test_fs_1/data</li></ul>

Here is an example lessfs configuration file (/mnt/vdisks/test_fs_1/lessfs.cfg):<br>
<pre><code>DEBUG=5<br>
HASHNAME=MHASH_TIGER192<br>
HASHLEN=24<br>
BLOCKDATA_IO_TYPE=file_io<br>
BLOCKDATA_PATH=/mnt/vdisks/test_fs_1/dta/blockdata.dta<br>
META_PATH=/mnt/vdisks/test_fs_1/mta<br>
META_BS=1048576<br>
CACHESIZE=512<br>
COMMIT_INTERVAL=10<br>
LISTEN_IP=127.0.0.1<br>
LISTEN_PORT=100<br>
MAX_THREADS=16<br>
DYNAMIC_DEFRAGMENTATION=on<br>
COREDUMPSIZE=2560000000<br>
SYNC_RELAX=0<br>
BACKGROUND_DELETE=on<br>
ENCRYPT_DATA=off<br>
ENCRYPT_META=off<br>
ENABLE_TRANSACTIONS=on<br>
BLKSIZE=131072<br>
COMPRESSION=snappy<br>
</code></pre>

Create the new lessfs file system:<br>
<pre><code>mklessfs -f -c /mnt/vdisks/test_fs_1/lessfs.cfg<br>
</code></pre>

Now add this line to your /etc/fstab file; be sure this line is <i>after</i> your normal, back-end file system that lessfs sits on top of:<br>
<pre><code>lessfs#/mnt/vdisks/test_fs_1/lessfs.cfg /mnt/vdisks/test_fs_1/data fuse defaults 0 0<br>
</code></pre>

You can now mount your lessfs file system:<br>
<pre><code>mount /mnt/vdisks/test_fs_1/data<br>
</code></pre>

You now have a file system that supports inline data de-duplication and can be used for virtual disk files (vdisk_fileio). If you are going to use lessfs in conjunction with mhVTL, you should use a separate storage device for the lessfs configuration and metadata (database files) since the VTL path is static. See the <a href='http://www.lessfs.com/wordpress/'>lessfs</a> web site for additional documentation and an explanation of the configuration parameters.<br>
<br>
<br>

<h3>Block Layer Caching</h3>
Several different block level (layer) caching solutions exist in ESOS. These software options allow you to use some type of fast storage (eg, an SSD) as a caching device to improve the performance of some other lower-end (probably large) storage. These would be similar or an alternative to controller-side (hardware) caching options like MegaRAID CacheCade or Adaptec maxCache. At the time of writing this, Enterprise Storage OS includes the following options:<br>
<ul><li>bcache <a href='http://bcache.evilpiepirate.org/'>http://bcache.evilpiepirate.org/</a>
</li><li>dm-cache <a href='http://visa.cs.fiu.edu/tiki/dm-cache'>http://visa.cs.fiu.edu/tiki/dm-cache</a>
</li><li>EnhanceIO <a href='https://github.com/stec-inc/EnhanceIO'>https://github.com/stec-inc/EnhanceIO</a>
</li><li>lvmcache <a href='https://sourceware.org/lvm2/'>https://sourceware.org/lvm2/</a></li></ul>

We'll attempt to give a brief setup example for each of these; please consult the project web sites above for additional information.<br>
<br>
<h4>bcache</h4>
For bcache, you'll first need to identify a caching device (like an SSD drive, or array of SSDs) and a backing device (a slow hard drive, or RAID array of spinning disks). The steps go something like this:<br>
<ol><li>Create and register the caching device.<br>
</li><li>Create and register the backing device.<br>
</li><li>Attach the caching device to the backing device.<br>
</li><li>You can now use the new bcache block device as any other block device in ESOS (eg, create a virtual disk file system for vdisk_fileio, use the raw block device for LVM, directly with vdisk_blockio, etc.).</li></ol>

Here is a real example of the commands for a bcache device; '/dev/sdc' is the caching device, '/dev/sdd' is the backing device, and the UUID comes from 'cset.uuid' in the bcache-super-show command:<br>
<pre><code>make-bcache -C /dev/sdc<br>
echo "/dev/sdc" &gt; /sys/fs/bcache/register<br>
make-bcache -B /dev/sdd<br>
echo "/dev/sdd" &gt; /sys/fs/bcache/register<br>
bcache-super-show /dev/sdc<br>
echo "6d4ab278-0844-4a50-8e74-87aeda4fd353" &gt; /sys/block/sdd/bcache/attach<br>
</code></pre>

You should now have a /dev/bcacheX device node that you can use.<br>
<br>
<h4>dm-cache</h4>
Now we'll take a look at setting up dm-cache. Two different block devices (or segments) are needed for dm-cache: (1) for the cache metadata, and (1) for the cache regions. For this example setup, we created both on one single SSD-backed volume using LVM. See <a href='http://blog.kylemanna.com/linux/2013/06/30/ssd-caching-using-dmcache-tutorial/'>this</a> article for a more in-depth example; for the example shown below, metadata size was not taken into account.<br>
<pre><code>pvcreate /dev/sda<br>
vgcreate ssd_vg /dev/sda<br>
lvcreate -L 10G -n ssd_metadata ssd_vg<br>
lvcreate -L 150G -n ssd_blocks ssd_vg<br>
pvcreate /dev/sdb<br>
vgcreate slow_disk_vg /dev/sdb<br>
lvcreate -L 100G -n data_vol slow_disk_vg<br>
blockdev --getsz /dev/mapper/slow_disk_vg-data_vol<br>
dmsetup create cached_dev --table '0 209715200 cache /dev/mapper/ssd_vg-ssd_metadata /dev/mapper/ssd_vg-ssd_blocks /dev/mapper/slow_disk_vg-data_vol 512 1 writeback default 0'<br>
</code></pre>

You would now have a '/dev/mapper/cached_dev' device node that can be used for a partition table & file system, raw block device, etc. To make dm-cache devices persist across reboots, you'll need to enable the rc script (rc.dmcache) in the /etc/rc.conf file. Then you'll need to add the commands to create/destroy the dm-cache device(s) using files "/etc/dm-cache.start" and "/etc/dm-cache.stop"; below are examples following suit from above.<br>
<br>
<code>/etc/dm-cache.start</code>:<br>
<pre><code>dmsetup create cached_dev --table '0 209715200 cache /dev/mapper/ssd_vg-ssd_metadata /dev/mapper/ssd_vg-ssd_blocks /dev/mapper/slow_disk_vg-data_vol 512 1 writeback default 0'<br>
dmsetup resume cached_dev<br>
</code></pre>

<code>/etc/dm-cache.stop</code>:<br>
<pre><code>dmsetup suspend cached_dev<br>
dmsetup remove cached_dev<br>
</code></pre>

<h4>EnhanceIO</h4>
The setup procedure for EnhanceIO cache devices is pretty clear-cut; the source or backing device (the device you want to "enhance") can already contain data and even have a mounted file system while adding/deleting a cache. The <code>eio_cli</code> tool that comes with ESOS is a special version that supports non-udev setups (like mdev in ESOS). EnhanceIO is disabled in ESOS by default; edit the /etc/rc.conf file and set <code>rc.eio_enable</code> to "YES". Next, you'll need to setup your cache device using the <code>eio_cli</code> tool (be sure to always use the "-u" option to disable support for udev):<br>
<pre><code>eio_cli create -u -d /dev/disk-by-id/SERIAL-B8CEA82A -s /dev/disk-by-id/SERIAL-A65CBA25 -m wb -c my_cache<br>
</code></pre>

Your backing/source device ("/dev/disk-by-id/SERIAL-B8CEA82A" in this example) is now enhanced! The configuration file that eio_cli and rc.eio use is located here: /etc/eio.conf<br>
<br>
<h4>lvmcache</h4>
You can also use the LVM interface to device-mapper cache (dm-cache). Using it via this method is much simpler compared to setting up dm-cache. First you'll need to make sure LVM is enabled for boot (set "rc.lvm2_enable" to "YES" in /etc/rc.conf).<br>
<br>
For this lvmcache setup example, we'll be using (1) SSD SCSI disk (our cache), and (1) 7.2K NL SAS SCSI disk (our backing disk).<br>
<br>
Make these SCSI disks into LVM PVs and add both devices to the same volume group (VG):<br>
<pre><code>pvcreate -v /dev/sdb /dev/sdc<br>
vgcreate -v VolumeGroup1 /dev/sdb /dev/sdc<br>
</code></pre>

You then need to create (3) logical volumes and allocate to each specific physical disk.<br>
<br>
Create a logical volume to use as cache and assign it to the SSD disk (/dev/sdb):<br>
<pre><code>lvcreate -L 950GB -n lv1_cache VolumeGroup1 /dev/sdb<br>
</code></pre>

Create a logical volume to use as the cache metadata and assign it to the SSD (/dev/sdb -- this needs to be about a 1000:1 split):<br>
<pre><code>lvcreate -L 1GB -n lv1_cache_meta VolumeGroup1 /dev/sdb<br>
</code></pre>

Create a logical volume to use as the data disk and assign it to the SAS 7.2K NL disk (/dev/sdc):<br>
<pre><code>lvcreate -L 2TB -n lv1_data VolumeGroup1 /dev/sdc<br>
</code></pre>

Now we need to convert the (2) cache volumes into a "cache pool" (this will add lv1_cache to a cache pool using lv1_cache_meta as the metadata):<br>
<pre><code>lvconvert --type cache-pool --poolmetadata VolumeGroup1/lv1_cache_meta VolumeGroup1/lv1_cache<br>
</code></pre>

Finally, attach the cache pool to the data volume -- your volume will now be cached:<br>
<pre><code>lvconvert --type cache --cachepool VolumeGroup1/lv1_cache VolumeGroup1/lv1_data<br>
</code></pre>

How to "un-cache" a logical volume... all you need to do is remove the cache pool logical volume (LVM will then copy the unwritten data to the data drive then remove the cache and metadata volumes):<br>
<pre><code>lvremove VolumeGroup1/lv1_cache<br>
</code></pre>

To add the cache back in, you will need to recreate the cache pool from scratch and assign it back to the logical volume.<br>
<br>
<br>

<h3>Automatic Tiered Block Devices (BTIER)</h3>
TODO<br>
<br>
Special thanks to Riccardo Bicelli for creating a BTIER resource agent (RA) for use with Pacemaker. This RA is included in ESOS; here is his original post for the BTIER RA: <a href='http://think-brick.blogspot.it/2014/09/btier-resource-agents-for-pacemaker.html'>http://think-brick.blogspot.it/2014/09/btier-resource-agents-for-pacemaker.html</a>

Example usage in ESOS:<br>
<pre><code>crm<br>
cib new btier<br>
configure primitive p_btier ocf:esos:btier \ <br>
params tier_devices="/dev/sda:/dev/sdb" \<br>
device_name="mybtierdev01"<br>
op monitor interval="10s"<br>
configure show<br>
cib commit btier<br>
quit<br>
</code></pre>

<br>

<h3>Ceph RBD Mapping</h3>
In ESOS, you can use a Ceph RBD image as a back-end block device (mapped to). You can then treat this as a normal block device and use it with vdisk_blockio, or put a file system on it and use vdisk_fileio.<br>
<br>
Edit the /etc/ceph/ceph.conf file and add your monitors (nodes) to the configuration file; this allows your Ceph cluster to be discovered. Here is an example:<br>
<pre><code>[mon.0]<br>
         host = node1<br>
         mon addr = 192.168.1.101:6789<br>
[mon.1]<br>
         host = node2<br>
         mon addr = 192.168.1.102:6789<br>
[mon.2]<br>
         host = node3<br>
         mon addr = 192.168.1.103:6789<br>
</code></pre>

You will also need a client key ring file (/etc/ceph/ceph.client.keyring):<br>
<pre><code>[client.admin]<br>
        key = AQC2WFlTYPvVHhAAuk1jxZ4u86EkMdeUyn6LYA==<br>
</code></pre>

Finally configure the /etc/ceph/rbdmap file (pool / image mappings):<br>
<pre><code>rbd/disk01     id=client.admin,keyring=/etc/ceph/ceph.client.keyring<br>
</code></pre>

Edit the /etc/rc.conf file and set "rc.rbdmap_enable" to "YES" and then start it:<br>
<pre><code>/etc/rc.d/rc.rbdmap start<br>
</code></pre>

If you get any error messages, check the kernel logs (<code>dmesg</code>). See this article if you have any "feature set mismatch" errors: <a href='http://cephnotes.ksperis.com/blog/2014/01/21/feature-set-mismatch-error-on-ceph-kernel-client/'>http://cephnotes.ksperis.com/blog/2014/01/21/feature-set-mismatch-error-on-ceph-kernel-client/</a>

<br>

<h3>File Systems & Virtual Disk Files</h3>
After you have setup/configured your advanced back-end storage, it will still appear as a block device, just as described in the basic back-storage wiki document.<br>
<br>
With this logical block device, you can now create a file system on it and create virtual disk files, if desired. Follow the same steps as described in the <a href='31_Basic_Back_End_Storage_Setup.md'>31_Basic_Back_End_Storage_Setup</a> document for making file systems and adding virtual disk files, but with the advanced back-storage, you'll select your DRBD block device (eg, /dev/drbd0) or whatever advanced block device you configured.<br>
<br>
<br>

<h3>Next Steps</h3>
You should now continue on to the <a href='41_Hosts_and_Initiators.md'>41_Hosts_and_Initiators</a> wiki page which will guide you through configuring SCST security groups.