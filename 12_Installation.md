### Install Prerequisites ###
You may now use a Linux, Windows, or Mac OS X machine to create a bootable ESOS USB flash drive.

Here is a list of the install requirements:
  * If using a Linux machine, it should be a recent release of a modern Linux distribution (32-bit or 64-bit), with common system tools (dd, cpio, etc.) installed.
  * If you're using a Windows host, the required binary programs (dd, ext2fsd, etc.) are all included in the ESOS install package.
  * A USB flash drive that is at least 4 GB large.
  * The ESOS image/install package available from the downloads page, or a ESOS disk image that you built from source.
  * For Mac OS X you'll need to install OSXFUSE and the ext2 FUSE module; see below for details.

_Hint: If you're using 'wget' to retrieve the installer package, add the '--content-disposition' flag so the file is named properly._

<br>

<h3>Installing ESOS (Linux)</h3>
If using a pre-built image package, you will need to extract it (version 0.1-<a href='https://code.google.com/p/enterprise-storage-os/source/detail?r=656'>r656</a> in our example):<br>
<pre><code>unzip esos-0.1-r656.zip<br>
</code></pre>

If using a ESOS image you built from source, you should now be in that directory, otherwise change to the package directory you just extracted:<br>
<pre><code>cd esos-0.1-r656<br>
</code></pre>

At this point, you should now plug in the USB flash drive you'd like to install ESOS to; you may need to unmount any file systems on your flash drive if you use a desktop-like Linux distribution (eg, Ubuntu).<br>
<br>
The <code>lsscsi</code> tool makes it very easy to find your USB drive SCSI device node (needed during the install). Here is an example of what that looks like:<br>
<pre><code>[root@localhost ~]# lsscsi <br>
[0:0:0:0]    disk    ATA      ST9160412AS      D005  /dev/sda <br>
[1:0:0:0]    cd/dvd  TSSTcorp DVD+-RW TS-L633C DW50  /dev/sr0 <br>
[8:0:0:0]    disk    Kingston DataTraveler G3  1.00  /dev/sdb <br>
</code></pre>

In the example above, we clearly recognize "Kingston DataTraveler G3" (hopefully) as our USB flash drive, and we can see the device node is "/dev/sdb" (far right column).<br>
<br>
You can now run the installer script (root privileges required):<br>
<pre><code>sudo ./install.sh<br>
</code></pre>

The <code>install.sh</code> script will verify the integrity of the ESOS disk image file, check for the required install tools, prompt for the desired SCSI device node to install ESOS on, and finally write the image to disk.<br>
<br>
After the image is completely written to your USB flash drive, then installation script will then ask you if you'd like to install any proprietary RAID controller CLI tools (optional). Some of these tools (eg, MegaCLI) are required for certain TUI functions in ESOS. The script will print the required files and download locations with instructions on where to place the files and complete the installation.<br>
<br>
If you have any installation issues, please use the esos-users Google Group.<br>
<br>
<br>

<h3>Installing ESOS (Windows)</h3>
The ESOS package (.zip archive) includes all tools required to create a ESOS USB flash drive using a Windows host. The install script has been tested on Windows 7 64-bit and Windows 8[.1] 64-bit machines.<br>
<br>
<ol><li>Download the desired install package from the ESOS downloads page.<br>
</li><li>Extract the .zip archive using Windows Explorer.<br>
</li><li>Insert your USB flash drive (>= 4 GB).<br>
</li><li>Browse to the extract package directory and run the "install.vbs" script.<br>
</li><li>Accept (click "Yes") when/if prompted for UAC privilege elevation.<br>
</li><li>Follow the interactive install script to complete your ESOS USB flash drive installation.</li></ol>

<br>

<h3>Installing ESOS (Mac OS X)</h3>
The standard Linux install script (install.sh) is also compatible for installing from a Mac OS X host. There are a couple requirements before running the <code>install.sh</code> script on Mac OS X:<br>
<ul><li>Install "OSXFUSE" (FUSE for OS X) from this page: <a href='http://osxfuse.github.io'>http://osxfuse.github.io</a>
</li><li>When installing the above package, choose the MacFUSE compatability option during install wizard.<br>
</li><li>Download and install the Ext2 FUSE module (fuse-ext2): <a href='http://fuse-ext2.sourceforge.net'>http://fuse-ext2.sourceforge.net</a></li></ul>

After completing the above requirements, you can then continue the ESOS USB flash drive installation. When downloading proprietary CLI tools on Mac OS X, you may need to disable Safari (or others) from extracting/opening the archives on download. The install script expects the archive files to be intact.<br>
<ol><li>Download the ESOS installation package from the project site.<br>
</li><li>Extract the archive (.zip file) if it hasn't already been done for you.<br>
</li><li>Open the Terminal application and change to the ESOS package directory: cd esos-trunk_r734<br>
</li><li>Next execute the ESOS installation script: ./install.sh<br>
</li><li>Follow the interactive install script to complete your ESOS USB flash drive installation.</li></ol>

<br>

<h3>Next Steps</h3>
Once you have successfully installed the ESOS image to a USB flash device, take the flash drive and plug it into your storage server. Then reboot the storage server and set USB devices to first (only) in the boot order. Then proceed to the <a href='21_Initial_System_Configuration.md'>21_Initial_System_Configuration</a> page.