### What is this? ###
This is the project home for Enterprise Storage OS™ (ESOS®). Enterprise Storage OS™ is a quasi Linux distribution based on the excellent [SCST](http://scst.sourceforge.net) project; its purpose is to provide SCSI targets via a compatible SAN (Fibre Channel, InfiniBand, iSCSI, FCoE). In a nutshell, ESOS® can easily turn a server with the appropriate hardware into a disk array that sits on your enterprise Storage Area Network (SAN) providing sharable block-level storage volumes. Typical uses for an ESOS® "storage server" include VMFS datastores on VMware ESX/ESXi, Windows NTFS volumes, Linux disks, etc.

<br>

<h3>ESOS® Features</h3>
Currently, ESOS® is in the early stages of development. The list of features is still growing; see the <a href='01_About_ESOS.md'>01_About_ESOS</a> wiki page for all of the details. Here are a few of the core ESOS® features:<br>
<ul><li>A high performance, purpose-built (appliance like) Linux base built from scratch, with no relation to other Linux distributions.<br>
</li><li>ESOS® is memory resident -- it boots off a USB flash drive, and everything is loaded into RAM. If the USB flash drive fails, ESOS® will send an alert email, and you can simply build a new ESOS® USB flash drive, then replace the failed drive and sync the configuration.<br>
</li><li>Kernel crash dump capture support. If the ESOS® Linux kernel happens to panic, the system will reboot into a crash dump kernel, capture the /proc/vmcore file to the esos_logs file system, and finally reboot back into the production ESOS® kernel -- all automatically. ESOS® sends an email alert on system start-up and checks for any crash dumps.<br>
</li><li>Two operating modes: Production (default) & Debug. With "Production" mode, the performance version of SCST (make 2perf) is used. If you find you're having a problem and not getting sufficient diagnostic logs, simply reboot into "Debug" mode (full SCST debug build, make 2debug) and get additional log data.<br>
</li><li>Enterprise RAID controller CLI configuration tools. Popular RAID controller CLI tools are an optional install with ESOS® (eg, LSI MegaRAID, Adaptec AACRAID, etc.) which allows configuration (add/delete/modify) of volumes / logical drives from a running ESOS® system.<br>
</li><li>ESOS® is compatible with most popular enterprise RAID controllers and Tier-1 server hardware. It currently supports the following front-end target types: Fibre Channel, iSCSI, InfiniBand (SRP), Fibre Channel over Ethernet (FCoE)<br>
</li><li>A text-based user interface (TUI) that provides an easy to use interface with convenient storage provisioning functions; see how it looks on the <a href='02_Screenshots.md'>02_Screenshots</a> wiki page.<br>
</li><li>Clustering / high availability (HA) components: Pacemaker + Corosync + DRBD<br>
</li><li>Create advanced back-end storage block device configurations using Linux software RAID (md) and Logical Volume Manager (LVM2).<br>
</li><li>Create virtual tape libraries (disk based) that can be used on your Storage Area Network (SAN). Works with popular software solutions such as Symantec NetBackup, Symantec BackupExec, EMC/Legato NetWorker, Bakbone Netvault, Tivoli Storage Manager (TSM), and Bacula. Support for VTLs in ESOS® is made possible via the mhVTL project.<br>
</li><li>Inline data de-duplication using lessfs; includes support for encryption and compression using QuickLZ, Google's Snappy, or LZO.<br>
</li><li>Support for Linux Ethernet bridging and NIC bonding (EtherChannel).<br>
</li><li>Software-based block layer cache solutions: bcache, dm-cache/lvmcache, and EnhanceIO.<br>
</li><li>Tiered storage devices with automatic migration and "smart" placement of data chunks via the BTIER project.<br>
</li><li>Support for using Ceph RBD images as back-end storage devices.<br>
</li><li>Advanced Fibre Channel over Ethernet (FCoE) support: ESOS® includes the fcst "software" FCoE target driver, and has the ability (build options) to support Emulex OCS FCoE CNA / Chelsio Uwire FCoE CNA hardware targets.</li></ul>

<br>

<h3>Getting Started</h3>
To get started setting up an ESOS® storage server, you will need a Linux host to use the ® USB drive installer, a USB flash drive, and a decent server with a good RAID card, disks, HBAs/HCAs/CNAs, etc. to use as the storage server -- see the <a href='03_Supported_Hardware.md'>03_Supported_Hardware</a> wiki page for more details. When you're ready to begin, grab the latest trunk snapshot package on this <a href='http://download.esos-project.com/packages/trunk/'>download</a> page and use the <a href='12_Installation.md'>12_Installation</a> document. See the <a href='11_Building.md'>11_Building</a> wiki page for directions on building ESOS® from source. Use the <a href='http://groups.google.com/group/esos-users'>esos-users</a> Google Group for any questions/comments.