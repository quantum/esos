### Introduction ###
Since ESOS [r512](https://code.google.com/p/enterprise-storage-os/source/detail?r=512) a new daemon was specifically written and integated into the install image, its purpose is to collect and store performance metrics into a Database.
While there are many existing tools which accomplish this, many of them require complex configuration directives and additional software to work correctly.

At current stage the daemon collects and stores block device metrics only, it can use PostgreSQL and MySQL as backends and takes care of compacting the samples to avoid excessive record numbers.

<br>

<h3>Configuring the Performance Statistics Agent</h3>
The agent uses a single configuration file: <code>/etc/perf-agent.con</code> which contains all of the relevant options. The database connection string is formatted using a standard URL notation. Example of Postgres and MySQL connection strings:<br>
<br>
<pre><code>#Postgres<br>
DBURI = postgres://username:password@host/database<br>
<br>
#MySQL<br>
DBURI = mysql://username:password@host/database<br>
</code></pre>

You need to provide the agent with an empty database, it will take care itself of creating all of the tables.<br>
<br>
The <code>System</code> option is used to identify your host in case of multiple ESOS agents logging to the same database. You can ignore the <code>HostAddress</code> option as it is not used.<br>
<br>
<code>PollingInterval</code> sets the samples resolution and by default it's equal to 5 seconds. Changing the resolution is strongly discouraged and it may later disappear as a configurable parameter.<br>
<br>
<code>BlockDevices</code> is a white-space separated list of devices to monitor.<br>
Example:<br>
<pre><code># Monitor /dev/sda /dev/sdb /dev/sdc<br>
BlockDevices = sda sdb sdc<br>
</code></pre>

<br>

<h3>Starting the Agent</h3>
The agent can be started by the init script:<br>
<pre><code>/etc/rc.d/rc.perfagent start<br>
</code></pre>

Or in debug mode (which will print messages to stdout):<br>
<pre><code>python /usr/local/perf-agent/perfagentmain.py<br>
</code></pre>
To terminate the agent in debug mode press <code>CTRL+C</code>

To start the agent automatically upon boot, change the <code>/etc/rc.conf</code> file as follows:<br>
<pre><code>#rc.perfagent_enable=NO<br>
rc.perfagent_enable=YES<br>
</code></pre>

<br>

<h3>Agent details</h3>
The following block device metrics are stored into the database, the first column is equal to the one in the database:<br>
<br>
<pre><code>readscompleted = BigInteger # n of read reqs completed<br>
readsmerged = BigInteger # n of reads merged by scheduler<br>
sectorsread = BigInteger # n of sectors read during sample period<br>
writescompleted = BigInteger # n of write requests completed<br>
sectorswritten = BigInteger # sectors written during period<br>
kbwritten = BigInteger # sum of Kb written during sample period<br>
kbread = BigInteger # sum of Kb read during sample period<br>
averagereadtime = Integer # avg of ms spent doing writes<br>
averagewritetime = Integer # avg of ms spent doing reads<br>
iotime = Integer # Combined I/O execution time in ms<br>
interval = Integer # Sample interval in s<br>
writespeed = Integer # W in Kb/s<br>
readspeed = Integer # R in Kb/s<br>
devicerate = Integer # Rate of combined R+W in KB/s<br>
</code></pre>

<br>

<h3>Auto-Compacter</h3>
Enabling the agent to start automatically on boot, will enable a sample reducer to be run once every 24 hours (<code>croncompact.py</code>), the reducer will compute averages of samples following this schema:<br>
<br>
<ul><li>Samples of the previous day (starting at 00:00 ending at 23:59) reduce to 15 minutes (average or sum depending of the field) and keep them for the next 7 days<br>
</li><li>Samples of 7 days ago (starting at 00:00 ending at 23:59) reduce to hourly samples<br>
</li><li>Samples of 31 days ago (starting at 00:00 ending at 23:59) reduce to 1 daily sample</li></ul>

If you don't want to reduce the samples or you will automatically purge them by other means then simply comment out the line in <code>/etc/crontab</code> which contains the croncompact.py reference.<br>
<br>
<br>

<h3>Nagios</h3>
TODO<br>
<br>
<br>