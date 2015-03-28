### Kernel Modules ###
The ESOS kernels (production / debug) are built with most drivers built-in. Generally the HBA, HCA, etc. drivers are left as modules in case the user would like to tweak any parameters via module options. The SCST core, device handlers, and target drivers are also modules.

If you do find you would like to customize any of the module parameters, first exit to the shell (Interface -> Exit to Shell) and then you can create the file `/etc/modprobe.conf` (which will be sync'd to USB if it exists). Below is an example to see the format of the file, we are not recommending you use these settings, its just a sample.
```
options scst scst_threads=20
options ib_srpt srpt_sq_size=32
```

You can see the valid parameters for a module using the `modinfo` command. See the [SCST](http://scst.sourceforge.net) documentation for possible information on tweaking any module parameters.

Synchronize your configuration:
```
conf_sync.sh
```

<br>

<h3>Extra Configuration</h3>
If you find that you need to run any extra commands before or after starting SCST (via RA, or rc/init), there are two files you can create/edit in /etc:<br>
<ul><li>/etc/pre-scst_xtra_conf<br>
</li><li>/etc/post-scst_xtra_conf<br>
These files can be treated as shell scripts; if either exists, they will be executed during start-up with <code>sh</code>.</li></ul>

The first file (/etc/pre-scst_xtra_conf) will be executed (if it exists) <i>before</i> any part of SCST is loaded. An example of something you may want to run before loading SCST is adjust the read-ahead value for a back-storage block device (eg, <code>blockdev --setra 2048 /dev/sdb</code>).<br>
<br>
The second file (/etc/post-scst_xtra_conf) will be executed (if it exists) <i>after</i> SCST is fully loaded and configured. If you want to adjust the SMP IRQ affinity for the system, this would probably be a good place for it.<br>
<br>
Synchronize your configuration:<br>
<pre><code>conf_sync.sh<br>
</code></pre>

<br>

<h3>High Availability (HA) / Clustering via the Pacemaker + Corosync Stack</h3>
A full featured cluster stack is included with ESOS. The stack consists of Pacemaker, Corosync, and crmsh. The resource-agents and fence-agents packages are also included along with other supporting tools/utilities (eg, Python, ipmitool, etc.).<br>
<br>
The setup and configuration of the cluster stack is well beyond the scope of this ESOS documentation, however, there is tons of information out there on Pacemaker/Corosync. We suggest starting with the following guides for configuration (obviously, all of the cluster components are already installed in ESOS):<br>
<ul><li><a href='http://clusterlabs.org/doc/en-US/Pacemaker/1.1-crmsh/html/Clusters_from_Scratch/index.html'>Clusters from Scratch</a>
</li><li><a href='http://clusterlabs.org/doc/en-US/Pacemaker/1.1-crmsh/html/Pacemaker_Explained/index.html'>Pacemaker Explained</a></li></ul>

The Pacemaker and Corosync rc scripts (rc.pacemaker & rc.corosync) are both disabled by default in ESOS. To enable them, edit the <code>/etc/rc.conf</code> file and set <code>rc.corosync_enable</code> and <code>rc.pacemaker_enable</code> to "YES". You'll then need to start both services:<br>
<ul><li>/etc/rc.d/rc.corosync start<br>
</li><li>/etc/rc.d/rc.pacemaker start</li></ul>

A SCST resource agent (ocf:esos:scst) is included with ESOS. It can be configured either as a normal resource (start/stop) or a multi-state resource (master/slave); the MS resource mode relies on the implicit Asymmetric Logical Unit Assignment (ALUA) functionality in SCST, so this must also be configured when using <code>ocf:esos:scst</code> as master/slave.<br>
<br>
When using <code>ocf:esos:scst</code> as a "normal" resource, the supporting user-land daemons and SCST modules are loaded when started, and when stopped, all daemons and modules are unloaded. This is important to know since when loading SCST, it expects whatever devices you have defined in /etc/scst.conf to be available (eg, /dev/drbd0 block device, or a virtual disk file on a file system). SCST will remove these from the configuration if they are not available when started; please keep this in mind when designing your cluster.<br>
<br>
<i>You will also want to make sure SCST start-up via init is disabled since the cluster stack will be managing it; set rc.scst_enable to 'NO' in /etc/rc.conf and stop the SCST service: /etc/rc.d/rc.scst stop</i>

An example of the SCST resource configuration for two ESOS nodes (one started, one stopped) might look something like this:<br>
<pre><code>crm<br>
cib new scst<br>
configure primitive p_scst ocf:esos:scst<br>
configure show<br>
cib commit scst<br>
quit<br>
</code></pre>

An example of the SCST resource configuration for two ESOS nodes (both started) might look something like this:<br>
<pre><code>crm<br>
cib new scst<br>
configure primitive p_scst ocf:esos:scst<br>
configure clone clone_scst p_scst \<br>
meta clone-max="2" clone-node-max="1" notify="true" interleave="true"<br>
configure show<br>
cib commit scst<br>
quit<br>
</code></pre>

If you want SCST to be loaded and running on the cluster nodes, but not necessarily "available" (path preference, see notes below) you can use <code>ocf:esos:scst</code> as a multi-state resource. The resource agent (RA) relies on ALUA in SCST to function. Below are example ALUA configurations in SCST for a two node cluster (SCST must be running in order to configure this, use <code>/etc/rc.d/rc.scst</code> to enable and then disable SCST).<br>
<br>
On the first host, using the shell (Interface -> Exit to Shell):<br>
<pre><code>/etc/rc.d/rc.scst start<br>
scstadmin -add_dgrp esos<br>
scstadmin -add_tgrp local -dev_group esos<br>
scstadmin -set_tgrp_attr local -dev_group esos -attributes group_id=1<br>
scstadmin -add_tgrp_tgt 21:00:00:e0:8b:9d:74:49 -dev_group esos -tgt_group local<br>
scstadmin -set_ttgt_attr 21:00:00:e0:8b:9d:74:49 -dev_group esos -tgt_group local -attributes rel_tgt_id=1<br>
scstadmin -add_tgrp remote -dev_group esos<br>
scstadmin -set_tgrp_attr remote -dev_group esos -attributes group_id=2<br>
scstadmin -add_tgrp_tgt 21:00:00:1b:32:01:6b:11 -dev_group esos -tgt_group remote<br>
scstadmin -set_ttgt_attr 21:00:00:1b:32:01:6b:11 -dev_group esos -tgt_group remote -attributes rel_tgt_id=2<br>
/etc/rc.d/rc.scst stop<br>
</code></pre>

On the second host, using the shell (Interface -> Exit to Shell):<br>
<pre><code>/etc/rc.d/rc.scst start<br>
scstadmin -add_dgrp esos<br>
scstadmin -add_tgrp local -dev_group esos<br>
scstadmin -set_tgrp_attr local -dev_group esos -attributes group_id=2<br>
scstadmin -add_tgrp_tgt 21:00:00:1b:32:01:6b:11 -dev_group esos -tgt_group local<br>
scstadmin -set_ttgt_attr 21:00:00:1b:32:01:6b:11 -dev_group esos -tgt_group local -attributes rel_tgt_id=2<br>
scstadmin -add_tgrp remote -dev_group esos<br>
scstadmin -set_tgrp_attr remote -dev_group esos -attributes group_id=1<br>
scstadmin -add_tgrp_tgt 21:00:00:e0:8b:9d:74:49 -dev_group esos -tgt_group remote<br>
scstadmin -set_ttgt_attr 21:00:00:e0:8b:9d:74:49 -dev_group esos -tgt_group remote -attributes rel_tgt_id=1<br>
/etc/rc.d/rc.scst stop<br>
</code></pre>

Now that ALUA is configured, you can configure the SCST resource. From this point forward, when you want your SCST devices to be used in the ALUA setup, you must run the following command command on each node for each device (DEVICE_NAME is the name of the SCST device):<br>
<pre><code>scstadmin -add_dgrp_dev DEVICE_NAME -dev_group esos<br>
</code></pre>

The SCST RA uses two ALUA states for Master/Slave; the 'active' state for Master, and the 'nonoptimized' state for Slave. The reasoning behind using <code>nonoptimized</code> is that we wanted the initiators to be able to pick a path themselves if something failed between it and the target (eg, switch, cable, HBA, etc.); this way it isn't required of the target (ESOS) to change ALUA state information based on an external path failure. This ALUA state seemed to work best in accomplishing this for most initiators.<br>
<br>
SCST multi-state resource example; two nodes (one master, one slave):<br>
<pre><code>crm<br>
cib new scst<br>
configure primitive p_scst ocf:esos:scst \<br>
params alua="true" \<br>
device_group=”esos” \<br>
local_tgt_grp=”local” \<br>
remote_tgt_grp=”remote”<br>
op monitor interval=”10” role="Master" \<br>
op monitor interval=”20” role="Slave" \<br>
op start interval="0" timeout="120" \<br>
op stop interval="0" timeout="60"<br>
ms ms_scst p_scst \<br>
meta master-max="1" master-node-max="1" \<br>
clone-max="2" clone-node-max="1" \<br>
notify="true"<br>
configure show<br>
cib commit scst<br>
quit<br>
</code></pre>

SCST multi-state resource example; two nodes (both master):<br>
<pre><code>crm<br>
cib new scst<br>
configure primitive p_scst ocf:esos:scst \<br>
params alua="true" device_group=”esos” \<br>
local_tgt_grp=”local” remote_tgt_grp=”remote” \<br>
m_alua_state="active" s_alua_state="nonoptimized" \<br>
op monitor interval=”10” role="Master" \<br>
op monitor interval=”20” role="Slave" \<br>
op start interval="0" timeout="120" \<br>
op stop interval="0" timeout="60"<br>
ms ms_scst p_scst \<br>
meta master-max="2" master-node-max="1" \<br>
clone-max="2" clone-node-max="1" \<br>
notify="true"<br>
configure show<br>
cib commit scst<br>
quit<br>
</code></pre>

<b><code>*</code><code>*</code> Warning: Care needs to be taken when using an active/active SCST configuration in MPIO and clustered initiator environments. SCST itself is not cluster aware and when using something like DRBD, it only replicates the blocks of data between hosts. There is more to SCSI than just reads/writes (locks, etc.) and these are <i>NOT</i> communicated to other hosts with SCST. If you are using some type of clustered/HA back-end storage and using ESOS/SCST as a different target type, these other SCSI items may be passed through the layers, but you should double-check. Even when using the SCST ALUA multi-state resource agent, it is <i>implicit</i> ALUA and the targets are only "suggesting" what path to take (assuming the initiator/application supports implicit ALUA). <i>Be sure not to use some type of round-robin algorithm for MPIO on you initiators!</i> Check your initiator configuration, and use a fixed pathing policy or something similar. <code>*</code><code>*</code></b>

Since ESOS makes use of email as a communication method, a email-helper script (external agent) was developed for the <code>crm_mon</code> utility. This is typically used with the <code>ocf:pacemaker:ClusterMon</code> resource agent in ESOS for cluster status change notifications. Below is an example of configuring it using crmsh; its recommended to not enable this resource until after you have completed setup and testing of your ESOS cluster as it can generate a lot of email messages.<br>
<br>
<pre><code>crm<br>
cib new notify<br>
configure primitive p_notify ocf:pacemaker:ClusterMon \<br>
params user="root" update="30" \<br>
extra_options="-E /usr/local/bin/crm_mon_email.sh -e root" \<br>
op monitor on-fail="restart" interval="10"<br>
configure clone clone_notify p_notify \<br>
meta target-role="Started"<br>
configure show<br>
cib commit notify<br>
quit<br>
</code></pre>

Synchronize your configuration:<br>
<pre><code>conf_sync.sh<br>
</code></pre>

<br>

<h3>InfiniBand Drivers</h3>
All of the InfiniBand drivers in ESOS are built as modules; they are all the in-line Linux kernel versions (not OFED). Most popular IB HCAs are supported including IP over InfiniBand (IPoIB). See the <a href='03_Supported_Hardware.md'>03_Supported_Hardware</a> wiki page for more details.<br>
<br>
ESOS uses a /etc/rc.d/rc.openibd rc script, similar to the OFED init script, openibd. Like the OFED version, we employ the use of the <code>/etc/infiniband/openib.conf</code> module configuration file. Edit this file to control which hardware drivers, and IB modules are loaded. <i>Note: IPoIB is disabled by default. When using Windows Server 2012 initiators with IPoIB, apparently Connected Mode (CM) is not supported.</i>

Synchronize your configuration:<br>
<pre><code>conf_sync.sh<br>
</code></pre>

<br>

<h3>InfiniBand Subnet Manager</h3>
The OpenSM IB subnet manager software is also included with ESOS. It is disabled by default, but can be enabled editing the /etc/rc.conf file and setting <code>rc.opensm_enable</code> to 'YES'.<br>
<br>
The OpenSM configuration files should be created in the /etc directory (/etc/opensm is the default location). For additional OpenSM IB subnet manager help/information, please see the related OFED documentation.<br>
<br>
Synchronize your configuration:<br>
<pre><code>conf_sync.sh<br>
</code></pre>

<br>

<h3>Enabling/Disabling System Services</h3>
Some of the ESOS system services can be enabled/disabled; typically this is needed if your specific configuration or setup has conflicting services. A common example is using DRBD on a stand-alone ESOS server, versus using DRBD in a ESOS cluster. In the latter scenario, you would typically want the cluster stack to manage the DRBD resources, so you would disable the <code>rc.drbd</code> service.<br>
<br>
Edit the "/etc/rc.conf" file and use 'YES' to enable a service, and 'NO' to disable a service. You can then use the following CLI command syntax to start/stop a service without rebooting:<br>
<pre><code>/etc/rc.d/rc.service_name [start | stop]<br>
</code></pre>

Synchronize your configuration:<br>
<pre><code>conf_sync.sh<br>
</code></pre>

<br>

<h3>NIC Bonding</h3>
Linux bonding (EtherChannel) is fully supported in ESOS; in order to configure a bond interface you'll need to initially create them using the shell (Interface -> Exit to Shell) like this (the name format bondX is a requirement):<br>
<pre><code>echo +bond0 &gt; /sys/class/net/bonding_masters<br>
</code></pre>

After your NIC bonding interfaces are created, you can then use the TUI to enslave interfaces to the bonding master interfaces (System -> Network Settings). When you choose a "Master" bonding interface in the TUI you can then select the slave interfaces. Then restart the networking when prompted to do so for the changes to take effect. <i>When you select another interface as a slave, if it has any current IP configuration set, those settings will be removed since it will now be a slave to a master bonding interface.</i>

NIC bonding interface parameters are also configured using the ESOS TUI in the Network Settings dialog (for a bonding master interface). The format for the "Bonding Options" box is exactly the same as RHEL's "BONDING_OPTS" setting. Use "key=value" pairs separated by a space (eg, "mode=1 miimon=100" in the text input box. See these pages for additional information on the Linux bonding driver parameters:<br>
<ul><li><a href='https://www.kernel.org/doc/Documentation/networking/bonding.txt'>https://www.kernel.org/doc/Documentation/networking/bonding.txt</a>
</li><li><a href='https://access.redhat.com/site/documentation/en-US/Red_Hat_Enterprise_Linux/6/html/Deployment_Guide/sec-Using_Channel_Bonding.html'>https://access.redhat.com/site/documentation/en-US/Red_Hat_Enterprise_Linux/6/html/Deployment_Guide/sec-Using_Channel_Bonding.html</a></li></ul>

<i><b>Be sure to use the naming format bondX for all NIC bonding interfaces!</b></i>

<br>

<h3>Ethernet Bridging</h3>
Linux bridge support is also enabled in ESOS. New bridge interfaces need to be created from the CLI (Interface -> Exit to Shell) before using the TUI to configure them.<br>
<br>
To add a new bridge interface (the name format brX is a requirement):<br>
<pre><code>brctl addbr br0<br>
</code></pre>

Once you've created a bridge, you can then exit the shell, and from the TUI configure the bridge interface (System -> Network Settings). Select the bridge interface then you can select the IP setting (DHCP or static) and select the bridge member interfaces. Apply the settings by restarting the network stack when prompted.<br>
<br>
<i><b>Be sure to use the naming format brX for all bridge interfaces!</b></i>

<br>

<h3>Ethernet Auto-negotiation (Speed/Duplex)</h3>
You can enable/disable auto-negotiating using the TUI to configure the speed/duplex settings of an interface manually. ESOS utilizes the <code>ethtool</code> utility to perform this configuration.<br>
<br>
To disable auto-negotiation and set the speed to "100 Mbps" and duplex to "full", set the following under the "ethtool options" in the TUI (System -> Network Settings):<br>
<pre><code>autoneg off speed 100 duplex full<br>
</code></pre>

You can also use this setting to configure additional interface options that are supported using "ethtool -s" -- see the <code>ethtool</code> manual page for details.<br>
<br>
<br>

<h3>What's Next</h3>
From here, you can now SSH into your ESOS storage server, configure your back-storage and setup your targets. Continue with the <a href='31_Basic_Back_End_Storage_Setup.md'>31_Basic_Back_End_Storage_Setup</a> document.