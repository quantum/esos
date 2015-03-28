### Read This First ###
Building ESOS from source is no longer the only option for installing/using ESOS; you may now download trunk snapshot packages that contain a pre-built disk image and everything you need to create a ESOS USB flash drive. If you'd still like to build ESOS from source, just for fun, or for developing ESOS, please continue...

<br>

<h3>Build Prerequisites</h3>
Here is a list of the build host system requirements:<br>
<ul><li>A recent release, of a modern x86-64 Linux distribution (64-bit Linux is required since ESOS uses only 64-bit kernel/programs).<br>
</li><li>All of the typical software development packages (eg, GCC, GNU Make, Autoconf, Libtool, etc.) are required.<br>
</li><li>A USB flash drive that is at least 4 GB -- not really required for building, but needed to install the ESOS "distribution" on (see <a href='12_Installation.md'>12_Installation</a> after completing the build).<br>
</li><li>Subversion is required to pull the source down to your machine.<br>
</li><li>The configure script will check for all of the required tools/utilities that are used directly by the ESOS Makefile.<br>
</li><li>Other tools, libraries, etc. that are required by the packages/projects that ESOS uses should be caught by those project's configure scripts.<br>
</li><li>Some free disk space for building, probably around 10 GB to be safe.<br>
</li><li>A lot of time! Unfortunately, several larger projects (GCC, Linux kernel, glibc, etc.) are required that take quite a while to compile.<br>
</li><li>Internet connectivity; packages are fetched from the dist. file repository.</li></ul>

Using <i>Red Hat Enterprise Linux (or CentOS) 6.2 Server x86_64</i> as your build host:<br>
<ul><li>Standard/basic install, defaults for everything.<br>
</li><li>yum groupinstall "Development tools"<br>
</li><li>yum install lsscsi kpartx libxslt bc wget gettext</li></ul>

Using <i>Fedora 16 Desktop Edition x86_64</i> as your build host:<br>
<ul><li>Standard/basic install, defaults for everything.<br>
</li><li>yum groupinstall "Development tools"<br>
</li><li>yum install wget lsscsi kpartx libxslt gettext-devel</li></ul>

Using <i>Ubuntu 12.04.2 LTS x86_64</i> as your build host:<br>
<ul><li>Standard/basic install, defaults for everything.<br>
</li><li>apt-get install build-essential<br>
</li><li>apt-get install subversion autoconf gawk flex bison gcc-multilib unzip libtinfo-dev libtool lsscsi kpartx libxslt1-dev groff gettext xsltproc pkg-config autopoint</li></ul>

Using <i>SUSE Linux Enterprise Desktop 11.4 x86_64</i> as your build host:<br>
<ul><li>Standard/basic install, defaults for everything.<br>
</li><li>zypper install -t pattern "devel_C_C++"<br>
</li><li>zypper install -t pattern "linux_kernel_devel"<br>
</li><li>zypper install subversion kpartx libxslt</li></ul>

Using <i>SUSE Linux Enterprise Server 12 x86_64</i> as your build host:<br>
<ul><li>Standard/basic install, defaults for everything.<br>
</li><li>zypper install -t pattern "SDK-C-C++"<br>
</li><li>zypper install subversion kpartx libxslt1 groff-full</li></ul>

<br>

<h3>Building ESOS</h3>
Grab the latest revision from trunk:<br>
<pre><code>svn checkout http://enterprise-storage-os.googlecode.com/svn/trunk esos<br>
cd esos<br>
</code></pre>

Generate the configure script:<br>
<pre><code>autoconf<br>
</code></pre>

After you have generated the configure script above, you then need to run the configure script (there aren't currently any options that affect anything build related):<br>
<pre><code>./configure<br>
</code></pre>

Finally, run make, which will fetch, extract, and build everything:<br>
<pre><code>make<br>
</code></pre>

After everything has been built, you will then need to create the disk image file (requires root privileges):<br>
<pre><code>sudo make image<br>
</code></pre>

You may now optionally create a package distribution (tarball) file if you desire:<br>
<pre><code>make pkg_dist<br>
</code></pre>

These are the same steps that the Buildbot host/software uses to create the SVN trunk snapshot packages that are available on the downloads page.<br>
<br>
<br>

<h3>Next Steps</h3>
Once you have successfully built your ESOS image/package, you can now move on to the <a href='12_Installation.md'>12_Installation</a> wiki page which will describe the install process. If you have any problems/questions building ESOS, please post to the esos-users Google Group.