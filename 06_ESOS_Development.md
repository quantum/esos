### Contributing to the ESOS Project ###
TODO

<br>

<h3>What's Needed</h3>
Here is a wish-list of improvements that need/should be done, or potential issues that need addressing; possible implementation ideas are also listed for some (in no particular order).<br>
<ul><li>Add support for Emulex FC HBAs; waiting on Emulex for updated lpfc to SCST shim (lpfc_scst) to work with newer kernels.<br>
</li><li>Speed up the ESOS build process. Possibly use the GNU make '--jobs' option to help.<br>
</li><li>Rework the conf_sync.sh script so its a bit more flexible; possibly sync. all of /etc and /var or something similar.<br>
</li><li>We use the device identification VPD page from devices for unique identifiers, but not all devices support this.<br>
</li><li>Newer Linux kernel releases no longer have updated firmware blobs; the driver references a version that does not exist in firmware/ and the user is expected to use the linux-firmware package for these referenced versions. This will need to be addressed in the next ESOS kernel version bump as some driver's firmware blobs are not compatible with the GPL, which means we probably can't re-distribute a binary ESOS image with these drivers. We will likely need to be more selective on what hardware/drivers can be used.<br>
</li><li>The current version of the kernel-side code for bcache does not pass any useful uevents. In linux-next, CACHED_UUID and CACHED_LABEL are uevents sent and then we can parse this in mdev setup and create unique sym. links for each bcache device.<br>
</li><li>We need a Windows installer (package) for ESOS; that is, we should have another install package download for Windows operating systems (1 for Linux, 1 for Windows). This should be relatively straight forward as ESOS is really just a disk image that we're writing to a USB flash drive.... but, the install script also adds in the proprietary RAID CLI tools (MegaCLI, etc.) so we'd need to extract tarballs, RPM archives, etc. in Windows. For the implementation I envision a install.vbs (VBScript) with the same flow as install.sh; we could put GPL'd binary tools for extracting tars, zips, RPM files, etc. into SVN and then the pkg_dist recipe will create a second distribution file for Windows that includes the VBScript installer, image, and binary tools.