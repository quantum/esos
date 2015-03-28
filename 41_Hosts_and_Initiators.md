### Host/Security Group Concept ###
The terms "security group", "host group", and "initiator group" are synonymous -- they are used interchangeably, likely throughout all of the ESOS documentation. Most enterprise-grade disk arrays have a similar concept. They provide initiator-orientated access control (eg, LUN masking). Each group is unique to a SCST target.

These groups can also be used to control I/O grouping in SCST. See the SCST documentation for more information. These "advanced" I/O group type settings can be configured via the CLI.

The idea is simple: For each target, you create a host group and then add your initiators and LUNs (mapped devices). The initiators and devices can then "see each other" (devices->LUNs).

Here is a typical example used in a VMware vSphere environment: (1) ESOS storage server with (2) Fibre Channel HBAs, each FC HBA is connected to an independent Fibre Channel switch (fabric); (3) ESXi hosts with (2) FC HBAs (initiators) each, one to each FC switch; you could then create (2) SCST host/security groups in ESOS, one for each target; then for each host group, you would be adding (3) initiators (the initiators connected to that target's fabric); finally you would map each SCST device to both groups.

<br>

<h3>Adding/Removing Groups</h3>
To add a new host/security group in the TUI choose the Add Group dialog (Hosts -> Add Group). You'll be prompted to select the target, and then choose a name for the new group.<br>
<br>
To remove a host/security group in the TUI choose the Remove Group dialog (Hosts -> Remove Group). Select the target when prompted, and then choose the group from the selection list that you'd like to remove.<br>
<br>
<br>

<h3>Adding/Removing Initiators</h3>
After you have created a new host group, you'll want to add one or more initiators to the group. Select the Add Initiator dialog in the TUI (Hosts -> Add Initiator). When prompted, choose the target that is associated with the group you'd like to add the initiator to. Next, select the desired host group.<br>
<br>
The initiator name should be the unique identifier for the remote initiator.<br>
<ul><li>For Fibre Channel initiators, this is the WWN (port name); it should be 8 bytes, with each byte separated by a ':' and lower-case alphabetic characters (eg, "10:00:00:00:c9:99:03:c3").<br>
</li><li>For iSCSI initiators, use the iSCSI Qualified Name (IQN) for the remote initiator (eg, "iqn.1991-05.com.microsoft:cm-1204-o-11.mccad.mcc.edu").<br>
</li><li>For InfiniBand initiators:<br>
<ul><li>By default in ESOS, the 'one_target_per_port' ib_srpt module option is set to '1'; in this mode, use the remote port GUID as the initiator name value (eg, "d949:560e:4ff0:0301:0023:7dff:ff95:169d").<br>
</li><li>Look in the kernel logs on your ESOS host for initiator's GUID.<br>
</li><li>If the 'one_target_per_port' option is set to '0' then append '0x' to the initiator GUID and remove the colons (':') from the value.<br>
</li></ul></li><li>The initiator name may also contain simple DOS-type patterns: '<code>*</code>', '<code>?</code>', '<code>!</code>'<br>
</li><li>The '<code>*</code>' means match all characters/symbols; the '<code>?</code>' means match a single character/symbol; the '<code>!</code>' can be used to revert the value of a pattern.</li></ul>

Use the Remove Initiator dialog (Hosts -> Remove Initiator) in the ESOS TUI to remove an initiator from a security group. <i>Be sure I/O is stopped for the remote initiator.</i>

<br>

<h3>Next Steps</h3>
Now that you have created your SCST security group(s) and added the initiator(s), you can move on to creating your SCST device and mapping the device as a LUN to one of these host groups. Go to the <a href='51_Device_Configuration.md'>51_Device_Configuration</a> page.