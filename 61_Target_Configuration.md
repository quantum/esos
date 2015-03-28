### Enabling/Disabling Targets ###
Enabling your target(s) is the final step to allow I/O (make target/volume visible to remote host). Open the Enable/Disable Target dialog in the TUI (Targets -> Enable/Disable Target). You can then choose the desired target and hit ENTER. Next you'll be shown the target's current state (enabled or disabled) and you can TAB to a radio widget to enable/disable the target. Hit 'OK' to make your selection.

<br>

<h3>Fibre Channel Targets</h3>
Fibre Channel targets in ESOS are considered "hardware-based" targets. The FC HBA is put into "target" mode and will be automatically available to SCST (ESOS). No additional configuration is required. At the time of writing this, only QLogic Fibre Channel Host Bus Adapters (HBAs) are built-in (default). If you happen to have Emulex SLI-4 Fibre Channel HBAs, you can build ESOS with the "--with-ocs_sdk" option (requires the Emulex OCS SDK) to support these FC HBAs as hardware target interfaces.<br>
<br>
For Fibre Channel targets, you can use the Issue LIP function in the TUI (Targets -> Issue LIP) to issue a LIP (Loop Initialization Primitive) on all Fibre Channel HBAs.<br>
<br>
<br>

<h3>InfiniBand Targets</h3>
InfiniBand (IB) targets are also considered "hardware-based" targets in ESOS; these will be available to SCST. Configuring the IB fabric (eg, subnet manager) is outside the scope of this documentation. See the <a href='03_Supported_Hardware.md'>03_Supported_Hardware</a> page for supported InfiniBand Host Channel Adapters (HCAs).<br>
<br>
<br>

<h3>iSCSI Targets</h3>
iSCSI targets in ESOS are considered to be "software-based" or dynamic targets. These targets need to be created initially and have no direct relationship to underlying hardware (similar to software iSCSI initiators).<br>
<br>
You can use the ESOS TUI to create iSCSI targets (Targets -> Add iSCSI Target). Creating a basic iSCSI target is very simple: Choose an IQN for the new target name, and hit the 'OK' button. ESOS generates an appropriate IQN automatically that you may use if you choose or you can change it to your liking.<br>
<br>
Removing iSCSI targets is just as simple as creating them in the TUI (Targets -> Remove iSCSI Target). Choose the target you'd like to delete and hit ENTER (you'll be prompted for confirmation). <i>This would definitely interrupt any I/O to/from the target!</i>

<br>

<h3>Hardware FCoE Targets (QLogic/Emulex/Chelsio FCoE CNA)</h3>
ESOS includes built-in support for QLogic FCoE CNAs via the QLogic/SCST qla2x00t target driver.<br>
<br>
Emulex FCoE CNAs (SLI-4) are supported via a build-time option (using "--with-ocs_sdk" with the autoconf script). This requires the Emulex OCS SDK (available from Emulex via registration).<br>
<br>
Chelsio FCoE CNAs are also supported using the "--with-uwire" option and requires the Chelsio Uwire driver package available from Chelsio's web site.<br>
<br>
All three of these driver types require no additional configuration on the ESOS side of things. They should appear as hardware target interfaces just like Fibre Channel and InfiniBand. You'll utilize the host/security groups for initiators like normal, and zone on your FCoE capable switches.<br>
<br>
<br>

<h3>Software FCoE Targets (via DCB/DCBX capable NIC)</h3>
DCB/DCBX capable NICs are supported via the "fcst" software FCoE target driver. Follow the configuration steps below. In addition, you'll need FCoE capable switches and other FC/FCoE supporting hardware/software. Simply having Ethernet switches that support DCB/DCBX and other FCoE technologies is not enough... FC name server, etc. are still required and not all "FCoE" switches/devices provide this -- check with your vendor.<br>
<br>
To configure the local interfaces on ESOS, you will first need to create the <code>/etc/fcoe/cfg-&lt;ifname&gt;</code> file (replacing <code>&lt;ifname&gt;</code> with your actual interface name). A default/example configuration file is included, simply copy this for each interface you'd like to setup; in this example our interface is 'eth2':<br>
<pre><code>cp /etc/fcoe/cfg-ethx /etc/fcoe/cfg-eth2<br>
</code></pre>

You can then edit this file to your liking (see <code>man 8 fcoemon</code> for more information).<br>
<br>
Next, restart the SCST service, which includes user-land daemons for FCoE (fcoemon, lldpad):<br>
<pre><code>/etc/rc.d/rc.scst stop &amp;&amp; /etc/rc.d/rc.scst start<br>
</code></pre>

Now you can configure the interface for DCB (Data Center Bridging):<br>
<pre><code>dcbtool sc eth2 dcb on<br>
dcbtool sc eth2 pfc e:1<br>
dcbtool sc eth2 app:fcoe e:1<br>
</code></pre>

Check the status and list of visible initiators with this command:<br>
<pre><code>fcc.sh<br>
</code></pre>

The FCoE target should now be available in SCST (fcst); it should be listed in the adapters information label in the main TUI screen and available under the Targets menu. You can now use this FCoE target the same as any other SCST target (creating host groups, adding initiators, etc.).<br>
<br>
<br>

<h3>Target Information</h3>
Some basic target information is accessible via the TUI using the Target Information dialog (Targets -> Target Information). Choose a target when prompted and hit the ENTER key to make a selection. Target information is display in a pop-up dialog (state, CPU mask, driver, etc.). Press ENTER to close the dialog.<br>
<br>
<br>

<h3>What's Next</h3>
At this point you've provisioned your storage and the remote hosts (initiators) should now have access to it. A rescan from initiator will likely be necessary to discover any new targets.<br>
<br>
If you're having any trouble, please read the <a href='04_ESOS_Support.md'>04_ESOS_Support</a> wiki page for basic help and troubleshooting tips. Use the <a href='http://groups.google.com/group/esos-users'>esos-users</a> Google Group if you are still not finding answers.