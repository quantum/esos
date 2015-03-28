### The First Boot ###
At this point its assumed you have already inserted your USB flash drive into your storage server's USB port, set USB as the only boot device in the BIOS, and you are at the physical console and have a login prompt. If you're ESOS USB flash drive failed to boot for some reason, please let us know on the esos-users Google Group so we can help.

On the first boot, you may get messages saying something like "Superblock last write time is in the future." -- just hit 'y' to fix it. This happens typically because the time on the build machine is/was different than your ESOS storage server machine.

The first item on your agenda will be to login as root and change the password.
  * Superuser account: root
  * Default password: esos

Next, **this is important**, you need to change the default root password: Hit 's' for the System menu, and then choose Change Password.

Choose a nice hefty password, assuming this machine will be on your IP network, you don't want it to be susceptible to easy SSH brute-force attacks.

<br>

<h3>Date & Time</h3>
Setting the date and time in ESOS is important; currently, the configuration synchronization script relies solely on time stamps to determine which files are "new" on the system. It will also help with log entry correlation when/if tracking down issues. Open the date & time dialog: System -> Date & Time Settings<br>
<br>
The default timezone for ESOS is Factory. Choose your local timezone, or set it UTC. You can then set the current date and time (24-hour format) if needed.<br>
<br>
Setting a NTP server is the easiest way to keep your ESOS system time up to date. The date/time will be set via NTP on ESOS host boot-up and periodically via cron.<br>
<br>
<b><code>*</code><code>*</code> Warning: If you do not set the date/time initially, and you do this after you've been using an ESOS instance (after the first sync), double-check the time stamps of files in the 'esos_conf' file system. If a file on the USB drive has a time stamp in the future from what your system time is, it will always "win" during a synchronization. <code>*</code><code>*</code></b>

<br>
<h3>TUI Basics</h3>
The ESOS text-based user interface (TUI) should be useful in any basic system setup, or storage provisioning functions depending on your hardware (RAID controller). Use the CLI (shell) for advanced configurations.<br>
<br>
When at the main menu (no dialogs or other widgets active) you can use the main menu hot keys to activate the main menu widget. These are first letter of each main menu option at the top of the screen: <b>S</b> -> System; <b>B</b> -> Back-End Storage; <b>D</b> -> Devices; <b>T</b> -> Targets; <b>H</b> -> Hosts; <b>I</b> -> Interface<br>
<br>
Pressing any of the hot keys will activate the main menu starting in the corresponding main menu heading. Use the arrow keys to navigate between main menu headings and sub-menu items. Use ENTER to select a function/dialog and ESCAPE to get out of the menu widget.<br>
<br>
When using selector/choice/list widgets (eg, selecting a logical drive), use ENTER or TAB to make a choice, ESCAPE to cancel/exit, and the arrow keys to navigate.<br>
<br>
Use the arrow keys to scroll up/down on scrolling window widgets which are typically used to display information/data on the screen (eg, DRBD Status). Use ENTER or ESCAPE to close these types of windows.<br>
<br>
On dialogs/screens that contain more than one widget, use TAB and SHIFT+TAB to traverse through the widgets. Use the OK button to confirm/accept/apply settings and Cancel to close/exit the dialog.<br>
<br>
<b><code>*</code><code>*</code> Warning: The ESOS TUI provides <i>minimal</i> human-error checking when configuring storage in ESOS (eg, deleting a volume that is mapped to a LUN). It is up to the end-user to make responsible decisions when administering ESOS hosts. <code>*</code><code>*</code></b>

<br>

<h3>Using the CLI</h3>
After logging into ESOS, you will be in the text-based user interface. You can then exit to the shell (CLI) if you choose: Interface -> Exit to Shell<br>
<br>
You can then return to the TUI by hitting CTRL-D or the <code>exit</code> command.<br>
<br>
<i>The only normal, interactive editor that is installed on ESOS is "vi".</i> For a handy <code>vi</code> cheat sheet, see <a href='http://www.lagmonster.org/docs/vi.html'>this</a> page.<br>
<br>
<b><code>*</code><code>*</code> Warning: Since the ESOS image (binaries, config. files, etc.) is loaded into RAM on boot, <i>it is important that you sync your configuration</i> to the USB flash drive by using the <code>conf_sync.sh</code> command. There is a cron job setup to run <code>conf_sync.sh</code> every so often, but it should be good practice to always run <code>conf_sync.sh</code> after making configuration changes via the shell. When using the TUI, it will automatically sync the configuration when you exit. <code>*</code><code>*</code></b>

<br>

<h3>Networking</h3>
Next, you will want to configure your network interface(s), system host/domain names, resolver, etc. If you are configuring IPoIB interfaces, you will need to first enable the kernel module(s); see the <a href='22_Advanced_System_Configuration.md'>22_Advanced_System_Configuration</a> wiki page for details. To access the network configuration dialog: System -> Network Settings<br>
<br>
Then choose General Network Settings. In this dialog, you can set the system host name, domain name, and if you're not using DHCP, the default gateway and name servers.<br>
<br>
Now you can configure your network interfaces (if desired). Go to the network settings dialog (System -> Network Settings) and the choose the desired network interface (eg, ethX).<br>
<br>
If you are using DHCP for an interface, choose the DHCP option in the radio widget, and change the default MTU if desired. When using DHCP, leave all of the other (eg, IP address, netmask, etc.) entry fields empty.<br>
<br>
If you are configuring an interface statically, choose the Static option and set the IP address, netmask, and broadcast addresses (all required). You may also change the default interface MTU if desired.<br>
<br>
After all networking settings have been configured appropriately, you can then restart the network services manually if you haven't already been prompted to do so: System -> Restart Networking<br>
<br>
If you are re-configuring network settings via SSH, please be careful when restarting the network services as you may lose connectivity if anything is not configured correctly.<br>
<br>
In ESOS, the <code>/etc/hosts</code> file is automatically updated each time the network rc script is run. If you have extra static host entries you'd like to use on your system, use the shell and create/edit the <code>/etc/xtra_hosts</code> file; the format is the same as the standard hosts file (IP_ADDRESS NAME).<br>
<br>
<br>

<h3>Mail Setup</h3>
It is highly recommended to configure mail (SMTP) on your ESOS storage server. Email messages are sent by many daemons in ESOS to communicate informational messages, warnings, errors, etc. ESOS uses sSMTP for sendmail -- it can only send mail, you cannot receive mail in ESOS. sSMTP in ESOS supports SSL/TLS encrypted connections to your SMTP host.<br>
<br>
Access the mail settings dialog: System -> Mail Setup<br>
<br>
All of the configurable options in the mail settings dialog should be self-explanatory. These settings will all be dependent on your SMTP service; please consult them for the appropriate settings. The alert email address field is the email address of the user that should receive the ESOS email alerts.<br>
<br>
Here are example settings for using Gmail SMTP:<br>
<pre><code>SMTP host: smtp.gmail.com<br>
SMTP port: 587<br>
Use TLS: Yes<br>
Use STARTTLS: Yes<br>
Auth. Method: CRAM-MD5<br>
Auth. User: username@gmail.com<br>
Auth. Password: password<br>
</code></pre>

After applying the ESOS mail settings (hitting 'OK') you will be prompted to send a test email message; choose 'Yes' and be sure your email setup is working.<br>
<br>
If you encounter any problems or errors when sending a test email message, consult /var/log/mail.log for help.<br>
<br>
You can also send a test email message at any time after: System -> Send Test Email<br>
<br>
<br>

<h3>Serial Console Access</h3>
Enterprise Storage OS supports console access via a serial port interface; here are the settings to configure on your terminal emulator:<br>
<ul><li>Baud: 9600<br>
</li><li>Data bits: 8<br>
</li><li>Parity bit: None<br>
</li><li>Stop bit: 1</li></ul>

For GRUB and kernel messages, use "COM1" (ttyS0). For ESOS shell access via serial, you'll need to edit the <code>/etc/inittab</code> file and add/uncomment a line for your serial device (eg, ttyS1). Then you need to make init re-read the file by performing this command:<br>
<pre><code>kill -HUP 1<br>
</code></pre>
The TUI is not available when connecting via serial, only the shell is usable.<br>
<br>
<br>

<h3>Shutting Down & Rebooting</h3>
To power off / shut down a running ESOS server, execute the following command from the shell:<br>
<pre><code>poweroff<br>
</code></pre>

To reboot a running ESOS server, execute the following command from the shell:<br>
<pre><code>reboot<br>
</code></pre>

<br>

<h3>What's Next</h3>
From here, you can now SSH into your ESOS storage server, configure your back-storage and setup your targets. Continue with the <a href='31_Basic_Back_End_Storage_Setup.md'>31_Basic_Back_End_Storage_Setup</a> document.<br>
<br>
If you have any extra settings, or non-standard configuration items, see the <a href='22_Advanced_System_Configuration.md'>22_Advanced_System_Configuration</a> wiki page for guidance.