### System Requirements ###
ESOS will run on any x86-64 platform; a 64-bit capable CPU is required (eg, AMD64 / Intel 64). SCST, the core of ESOS, is fully multi-threaded with complete SMP support, meaning the more sockets/cores you have, and the faster your CPUs, the more IO that can be processed.

Enterprise-grade server hardware is definitely recommended for your ESOS disk array. Obviously redundant power supplies, multiple HBAs/HCAs/NICs, fault-tolerant RAID back-storage, memory sparring/mirroring, etc. should all be must-haves for production deployments.

In addition to the above, the following are required to run ESOS:
  * At least 4 GB of usable, physical RAM.
  * A USB flash drive that is at least 4 GB.

_If you have any hardware that you would like supported that is not listed below, please submit a request to the [esos-users](http://groups.google.com/group/esos-users) Google Group._

<br>

<h3>Local RAID Controllers</h3>
While there is support for a number of different RAID controllers, not all brands include the CLI tools that allow creating/destroying/etc. volumes from inside ESOS. If your favorite RAID controller has a CLI tool, please let us know so we can consider including it with ESOS.<br>
<ul><li>LSI Logic MegaRAID cards (CLI tool: MegaCli64 / storcli64)<br>
<ul><li>Including re-branded MegaRAID compatible adapters: Dell PERC; IBM ServeRAID; Sun/Oracle, Cisco, and Intel RAID controllers<br>
</li></ul></li><li>Adaptec AACRAID cards (CLI tool: arcconf)<br>
<ul><li>Including re-branded AACRAID compatible adapters: IBM ServeRAID; Sun/Oracle RAID controllers<br>
</li></ul></li><li>HP Smart Array cards (CLI tool: hpacucli)<br>
</li><li>Areca SATA/SAS RAID cards (CLI tool: cli64)<br>
</li><li>3ware 9000-series SATA and 9700-series SATA/SAS RAID cards (CLI tool: tw_cli.x86_64)</li></ul>

<br>

<h3>Fibre Channel HBAs / FCoE CNAs (Target, Initiator Support)</h3>
QLogic FC HBAs and FCoE CNAs are fully supported in ESOS. Emulex FC/FCoE products based on SLI-4 are supported via the OneCore Storage SDK from Emulex (manual build required). Chelsio T5 10/40 GbE FCoE adapters are supported via the Unified Wire driver (manual build required).<br>
<ul><li>QLogic QLA2XXX FC Host Bus Adapters (QLE2460, QLE2560, QLA2340, etc.)<br>
</li><li>QLogic QLE8XXX FCoE Converged Network Adapters (QLE8360, QLE8362, etc.)<br>
</li><li>Emulex XE201 IOC combo adapters (LPe15004, LPe16000B, LPe16202, and LPe16004)<br>
</li><li>Emulex XE100-series 10/40 GbE CNAs (OCe14102-UX, OCe14102-UM, OCe14101-NX, OCe14102-NX, and OCe14401-NX)<br>
</li><li>Chelsio T5 UWire CNAs (T520, T580, T540, and T522)</li></ul>

<br>

<h3>InfiniBand HCAs (Target, Initiator Support)</h3>
Several different brand/model IB Host Channel Adapters work with ESOS.<br>
<ul><li>Mellanox InfiniHost Host Channel Adapters (MT23108, MT25208, etc.)<br>
</li><li>Mellanox ConnectX PCI Express HCAs (MNPH29D-XTR, MHGH28-XTC, etc.)<br>
</li><li>QLogic HTX Host Channel Adapter (QHT7140)<br>
</li><li>Intel PCIe QLE InfiniBand HCAs (QLogic QLE7340, QLE7280, etc.)<br>
</li><li>Chelsio iWARP/RDMA T3 1/10 GbE adapters<br>
</li><li>Cehlsio iWARP/RDMA T4 1/10 GbE adapters<br>
</li><li>NetEffect Ethernet Cluster Server Adapters (RNIC driver)</li></ul>

<br>

<h3>Network Interface Cards / CNAs (Initiator, NIC Support)</h3>
Although Linux provides drivers for a ton of different NICs, we tried to keep it simple and only support NICs that are common in enterprise IT environments. The list below includes popular 10 GbE and 1 GbE cards, and even a couple lower-end cards that might be useful for console/configuration access (SSH).<br>
<ul><li>Intel PRO/1000 Gigabit Ethernet adapters<br>
</li><li>Intel PRO/1000 PCI-Express Gigabit Ethernet adapters<br>
</li><li>Intel 82575/82576 PCI-Express Gigabit Ethernet adapters<br>
</li><li>Broadcom Tigon3 adapters<br>
</li><li>Broadcom NetXtremeII adapters (with CNIC support)<br>
</li><li>QLogic QLA3XXX adapters<br>
</li><li>Chelsio T1/T3/T4 1 GbE and 10 GbE adapters (with T3/T4 initiator iSCSI offload support)<br>
</li><li>Cisco VIC Ethernet NIC adapter<br>
</li><li>Intel 10GbE PCI Express adapters<br>
</li><li>Intel PRO/10GbE adapters<br>
</li><li>Mellanox Technologies ConnectX 10Gbit Ethernet adapters<br>
</li><li>Broadcom NetXtremeII 10Gb cards (with iSCSI/FCoE initiator offload support)<br>
</li><li>QLogic QLCNIC 1/10Gb CNAs (QLE8240, QLE8242)<br>
</li><li>QLogic QLGE ISP8XXX 10Gb Ethernet adapters<br>
</li><li>Brocade 1010/1020 10Gb Ethernet cards<br>
</li><li>Intel(R) PRO/100+ Ethernet cards<br>
</li><li>RealTek RTL-8139 C+ PCI Fast Ethernet adapters<br>
</li><li>RealTek RTL-8129/8130/8139 PCI Fast Ethernet adapters<br>
</li><li>Realtek 8169 Gigabit Ethernet cards<br>
</li><li>Marvell Yukon Gigabit Ethernet cards<br>
</li><li>Marvell Yukon 2  Ethernet adapters