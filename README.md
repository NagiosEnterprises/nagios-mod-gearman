Nagios Gearman Module
=====================

What is Nagios-Mod-Gearman
-------------------

Nagios-Mod-Gearman is an easy way
of distributing active Nagios checks across your network and
increasing Nagios scalability. Nagios-Mod-Gearman can even help to reduce the
load on a single Nagios host, because its much smaller and more
efficient in executing checks.

 * Nagios-Mod-Gearman works with Nagios 4.5.x

It consists of three parts:

 * There is a NEB module which resides in the Nagios core and adds servicechecks,
   hostchecks and eventhandler to a Gearman queue.
 * The counterpart is one or more worker clients executing the checks.
   Worker can be configured to only run checks for specific host- or
   servicegroups.
 * And you need at least one http://gearman.org[Gearman Job Server]
   running.
 * See the <<common-scenarios,common scenarios>> for some examples.


Features
--------
 * Reduce load of your central Nagios machine
 * Make Nagios scalable up to thousands of checks per second
 * Easy distributed setups without configuration overhead
 * Real loadbalancing across all workers
 * Real failover for redundant workers
 * Fast transport of passive check results with included tools like
   nagios-send-gearman and nagios-send-multi


Support
-------
 * Nagios-Mod-Gearman has been succesfully tested with latest Nagios.
 * There are no known bugs at the moment. Let us know if you find one.


Changelog
---------
The changelog is available in the tarball


How does it work
----------------
When the Nagios-Mod-Gearman broker module is loaded, it intercepts all
servicechecks, hostchecks and the eventhandler events. Eventhandler
are then sent to a generic 'eventhandler' queue. Checks for hosts
which are in one of the specified hostgroups, are sent into a separate
hostgroup queue. All non matching hosts are sent to a generic 'hosts'
queue.  Checks for services are first checked against the list of
servicegroups, then against the hostgroups and if none matches they
will be sent into a generic 'service' queue. The NEB module starts a
single thread, which monitors the 'check_results' where all results
come in.

A simple example queue would look like:

----
+---------------+------------------+--------------+--------------+
| Queue Name    | Worker Available | Jobs Waiting | Jobs Running |
+---------------+------------------+--------------+--------------+
| check_results | 1                | 0            | 0            |
| eventhandler  | 50               | 0            | 0            |
| host          | 50               | 0            | 1            |
| service       | 50               | 0            | 13           |
+---------------+------------------+--------------+--------------+
----

There is one queue for the results and two for the checks plus the
eventhandler queue.

The workflow is simple:

 1. Nagios wants to execute a service check.
 2. The check is intercepted by the Nagios-Mod-Gearman neb module.
 3. Nagios-Mod-Gearman puts the job into the 'service' queue.
 4. A worker grabs the job and puts back the result into the
    'check_results' queue
 5. Nagios-Mod-Gearman grabs the result job and puts back the result onto the
    check result list
 6. The Nagios reaper reads all checks from the result list and
    updates hosts and services


You can set some host or servicegroups for special worker. This
example uses a separate hostgroup for Japan and a separate
servicegroup for resource intensive selenium checks.

It would look like this:

----
+-----------------------+------------------+--------------+--------------+
| Queue Name            | Worker Available | Jobs Waiting | Jobs Running |
+-----------------------+------------------+--------------+--------------+
| check_results         | 1                | 0            | 0            |
| eventhandler          | 50               | 0            | 0            |
| host                  | 50               | 0            | 1            |
| hostgroup_japan       | 3                | 1            | 3            |
| service               | 50               | 0            | 13           |
| servicegroup_selenium | 2                | 0            | 2            |
+-----------------------+------------------+--------------+--------------+
----

You still have the generic queues and in addition there are two queues
for the specific groups.


The worker processes will take jobs from the queues and put the result
back into the check_result queue which will then be taken back by the
neb module and put back into the Nagios core. A worker can work on one
or more queues. So you could start a worker which only handles the
'hostgroup_japan' group.  One worker for the 'selenium' checks and one
worker which covers the other queues. There can be more than one
worker on each queue to share the load.


Common Scenarios
----------------

Load Balancing
~~~~~~~~~~~~~~

The easiest variant is a simple load balancing. For example if your
single Nagios box just cannot handle the load, you could just add a
worker in the same network (or even on the same host) to reduce your
load on the Nagios box. Therefor we just enable hosts, services and
eventhandler on the server and the worker.

Pro:

 * reduced load on your monitoring box

Contra:

 * no failover



Distributed Monitoring
~~~~~~~~~~~~~~~~~~~~~~

If your checks have to be run from different network segments, then
you can use the hostgroups (or servicegroups) to define a hostgroup
for specific worker. The general hosts and services queue is disabled
for this worker and just the hosts and services from the given
hostgroup will be processed.

Pro:

 * reduced load on your monitoring box
 * ability to access remote networks

Contra:

 * no failover



Distributed Monitoring with Load Balancing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Your distributed setup could easily be extended to a load balanced
setup with just adding more worker of the same config.

Pro:

 * reduced load on your monitoring box
 * ability to access remote networks
 * automatic failover and load balancing for worker

Contra:

 * no failover for the master



NSCA Replacement
~~~~~~~~~~~~~~~~

If you just want to replace a current NSCA solution, you could load
the Nagios-Mod-Gearman NEB module and disable all distribution features. You
still can receive passive results by the core send via
nagios-send-gearman / nagios-send-multi. Make sure you use the same encryption
settings like the neb module or your core won't be able to process the
results or use the 'accept_clear_results' option.

Pro:

 * easy to setup in existing environments



Distributed Setup With Remote Scheduler
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


In case your network is unstable or you need a gui view from the
remote location or any other reason which makes a remote core
unavoidable you may want this setup. This setup consists of 2
independent Nagios-Mod-Gearman setups and the slave worker just send their
results to the master via the 'dup_server' option. The master
objects configuration must contain all slave services and hosts.
The configuration sync is not part of Nagios-Mod-Gearman.

Pro:

 * independent from network outtakes
 * local view

Contra:

 * more complex setup
 * requires configuration sync



Installation
------------

Debian / Ubuntu
~~~~~~~~~~~~~~~
It is strongly recommended to use the DEB files in the Build folder associated
with the OS you are installing on, it contains the prebuilt Debian and Ubuntu packages.

Building packages from source.

Extract the tarball

tar xzf nagios-mod-gearman-1.0.1.tar.gz

To setup the development, the following packages need to be installed by running this as root.

apt install automake libncurses-dev gearman libgearman-dev help2man dctrl-tools libperl-dev g++ libltdl-dev pkgconf make dpkg-dev debhelper libssl-dev

Run the following as root to compile Nagios-Mod-Gearman and to install it.

cd nagios-mod-gearman-1.0.1
chmod a+x autogen.sh
./autogen.sh
./configure
make install

To build the deb packages, just run the following.
make deb

The debian builds the packages into 3 different deb files.

To install just the worker using the deb packages, only this package needs to be installed. (replace x.x.x with the version number)
apt-get install /<pathtothedirectory>/nagios-mod-gearman-worker_x.x.x_amd64.deb

To install the nagios-gearman-top nagios-check-gearman tools, this package is installed. (replace x.x.x with the version number)
apt-get install /<pathtothedirectory>/nagios-mod-gearman-tools_x.x.x_amd64.deb

To install the broker on the nagios server, this package is installed. (replace x.x.x with the version number)
apt-get install /<pathtothedirectory>/nagios-mod-gearman-module_x.x.x_amd64.deb

Example command.
apt-get install /<pathtothedirectory>/nagios-mod-gearman-module_x.x.x_amd64.deb
apt-get install /<pathtothedirectory>/nagios-mod-gearman-tools_x.x.x_amd64.deb
apt-get install /<pathtothedirectory>/nagios-mod-gearman-worker_x.x.x_amd64.deb

To enable the worker to start at boot and to start it, run this.
systemctl enable nagios-mod-gearman-worker
systemctl start nagios-mod-gearman-worker

To enable Nagios Core to load the Nagios Mod Grarman broker, edit the /usr/local/nagios/etc/nagios.cfg file and add the following line to the bottom of the file.

broker_module=/usr/lib64/nagios-mod-gearman/nagios-mod-gearman.o config=/etc/nagios-mod-gearman/module.conf eventhandler=no

Then restart nagios core
systemctl restart nagios


Centos Stream / Redhat / Oracle Linux
~~~~~~~~~~~~~
The easiest way to install Nagios Mod Gearman is to build RPM packages.

It is strongly recommended to use the RPM files in the Build folder associated
with the OS you are installing on, it contains the prebuilt RHEL 8 and RHEL 9 packages.

Building packages from source.

Extract the tarball

tar xzf nagios-mod-gearman-1.0.1.tar.gz

To setup the development, the following packages and repositories need to be installed by running this as root.

CentOS 9

yum install epel-release -y
dnf config-manager --set-enabled crb

RHEL 8

yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

RHEL 9

yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
subscription-manager repos --enable codeready-builder-for-rhel-9-x86_64-rpms

yum install autoconf automake libtool boost-devel boost-program-options libgearman libgearman-devel libtool-ltdl-devel ncurses-devel rpm-build gearmand

Run the following as root to compile Nagios-Mod-Gearman and to install it.

cd nagios-mod-gearman-1.0.1
chmod a+x autogen.sh
./autogen.sh
./configure
make
make install

To build the rpm packages, just run the following.
make rpm

To install the rpm, run this (replace x.x.x with the version number and the os version change elx)
yum localinstall nagios-mod-gearman-x.x.x-1.elx.x86_64.rpm

To enable the worker to start at boot and to start it, run this.
systemctl enable nagios-mod-gearman-worker
systemctl start nagios-mod-gearman-worker

To enable Nagios Core to load the Nagios Mod Grarman broker, edit the /usr/local/nagios/etc/nagios.cfg file and add the following line to the bottom of the file.

broker_module=/usr/lib64/nagios-mod-gearman/nagios-mod-gearman.o config=/etc/nagios-mod-gearman/module.conf eventhandler=no

Then restart nagios core
systemctl restart nagios


Install Gearmand on the Nagios server
--------------------------------------
RHEL / Centos Stream / Oracle Linux

If you downloaded all of the packages in the previous step you should have gearmand installed and just need to enable and start it

The gearmand package is available on the Extra Packages for Enterprise Linux (EPEL) repository so the server needs to have that available.

To install the latest gearmand package on the Nagios server, run this as root.
dnf install gearmand

Enable gearmand to start at boot and to restart it, run this as root.
systemctl enable gearmand
systemctl start gearmand



Ubuntu / Debian

To install the latest gearmand package on the Nagios server, run this as root.

apt install gearman-job-server

Gearmand is only setup to listen on localhost which needs to be changed to allow the remote workers to connect to the gearman server.
Edit the /etc/default/gearman-job-server file on the server. Delete the following section from the file.
--listen=localhost

Restart gearmand so it will listen for any worker connections.

Enable gearmand to start at boot and to restart it, run this as root.
systemctl enable gearman-job-server
systemctl restart gearman-job-server


Configuration
-------------

Nagios Core module options.
~~~~~~~~~~~

See the following list for a detailed explanation of available
options:

Common Options to modify in the config.
~~~~~~~~~~~~~~

    config=/etc/nagios-mod-gearman/module.conf

=====

debug::
# use debug to increase the verbosity of the module.
# Possible values are:
#     0 = only errors
#     1 = debug messages
#     2 = trace messages
#     3 = trace and all gearman related logs are going to stdout.
# Default is 0.
  - trace and all gearman related logs are going to stdout
+
Default is 0.
+
====
    debug=0
====


logfile::
Path to the logfile.
+
====
    logfile=/var/log/nagios-mod-gearman/nagios-mod-gearman-neb.log
====


server::
sets the address of your gearman job server. Can be specified
more than once to add more server. Nagios-Mod-Gearman uses
the first server available.
+
====
    server=localhost:4730,remote_host:4730
====


eventhandler::
defines if the module should distribute execution of
eventhandlers.
+
====
    eventhandler=no
====


services::
defines if the module should distribute execution of service checks.
+
====
    services=no
====


hosts::
defines if the module should distribute execution of host checks.
+
====
    hosts=no
====


hostgroups::
sets a list of hostgroups which will go into separate queues.
+
====
    hostgroups=name1,name2,name3
====


servicegroups::
sets a list of servicegroups which will go into separate queues.
+
====
    servicegroups=name1,name2,name3
====


encryption::
enables or disables encryption. It is strongly advised to not disable
encryption. Anybody will be able to inject packages to your worker. Encryption
is enabled by default and you have to explicitly disable it. When using
encryption, you will either have to specify a shared password with `key=...` or
a keyfile with `keyfile=...`.
Default is On.
+
====
    encryption=yes
====

key::
A shared password which will be used for encryption of data packets. Should be at
least 8 bytes long. Maximum length is 32 characters.
+
====
    key=should_be_changed
====

keyfile::
The shared password will be read from this file. Use either key or keyfile.
Only the first 32 characters from the first line will be used.
Whitespace to the right will be trimmed.
+
====
    keyfile=/path/to/secret.file
====

use_uniq_jobs::
Using uniq keys prevents the gearman queues from filling up when there
is no worker. However, gearmand seems to have problems with the uniq
key and sometimes jobs get stuck in the queue. Set this option to 'off'
when you run into problems with stuck jobs but make sure your worker
are running.
Default is On.

+
====
    use_uniq_jobs=on
====

gearman_connection_timeout::
Timeout in milliseconds when connecting to gearmand daemon. Default value is
defined as 'no timeout'.
Default is -1.

+
====
    gearman_connection_timeout=-1
====

Server Options
~~~~~~~~~~~~~~

Additional options for the NEB module only:

localhostgroups::
sets a list of hostgroups which will not be executed by nagios-mod-gearman. They are just
passed through.
+
====
    localhostgroups=name1,name2,name3
====


localservicegroups::
sets a list of servicegroups which will not be executed by gearman. They are
just passed through.
+
====
    localservicegroups=name1,name2,name3
====


queue_custom_variable::
Can be used to define the target queue by a custom variable in
addition to host/servicegroups. When set for ex. to 'WORKER' you then
could define a '_WORKER' custom variable for your hosts and services
to directly set the worker queue. The host queue is inherited unless
overwritten by a service custom variable. Set the value of your custom
variable to 'local' to bypass Nagios-Mod-Gearman (Same behaviour as in
localhostgroups/localservicegroups).
+
====
    queue_custom_variable=WORKER
====



do_hostchecks::
Set this to 'no' if you want Nagios-Mod-Gearman to only take care of
servicechecks. No hostchecks will be processed by Nagios-Mod-Gearman. Use
this option to disable hostchecks and still have the possibility to
use hostgroups for easy configuration of your services.
If set to yes, you still have to define which hostchecks should be
processed by either using 'hosts' or the 'hostgroups' option.
Default: `yes`
+
====
    do_hostchecks=yes
====


result_workers::
Enable or disable result worker thread. The default is one, but
you can set it to zero to disabled result workers, for example
if you only want to export performance data.
+
====
    result_workers=1
====


perfdata::
Defines if the module should distribute perfdata to gearman.
Can be specified multiple times and accepts comma separated lists.
+
====
    perfdata=no
====
NOTE: processing of perfdata is not part of Nagios-Mod-Gearman. You will need
additional worker for handling performance data. For example:
http://www.pnp4nagios.org[PNP4Nagios]. Performance data is just
written to the gearman queue.


perfdata_send_all::
Set 'perfdata_send_all=yes' to submit all performance data
of all hosts and services regardless of if they
have 'process_performance_data' enabled or not.
Default: `no`
+
====
    perfdata_send_all=no
====


perfdata_mode::
There will be only a single job for each host or service when putting
performance data onto the perfdata_queue in overwrite mode. In
append mode perfdata will be stored as long as there is memory
left. Setting this to 'overwrite' helps preventing the perf_data
queue from getting to big. Monitor your perfdata carefully when
using the 'append' mode.
Possible values are:
+
--
    * `1` - overwrite
    * `2` - append
--
+
Default is 1.
+
====
    perfdata_mode=1
====


result_queue::
sets the result queue. Necessary when putting jobs from several Nagios instances
onto the same gearman queues. Default: `check_results`
+
====
    result_queue=check_results
====


orphan_host_checks::
The Nagios-Mod-Gearman NEB module will submit a fake result for orphaned host
checks with a message saying there is no worker running for this
queue. Use this option to get better reporting results, otherwise your
hosts will keep their last state as long as there is no worker
running.
Default is yes.
+
====
    orphan_host_checks=yes
====


orphan_service_checks::
Same like 'orphan_host_checks' but for services.
Default is yes.
+
====
    orphan_service_checks=yes
====


orphan_return::
Set return code of orphaned checks.
Possible values are:
+
--
    * `0` - OK
    * `1` - WARNING
    * `2` - CRITICAL
    * `3` - UNKNOWN
--
+
Default is 2.
+
====
    orphan_return=2
====


accept_clear_results::
When enabled, the NEB module will accept unencrypted results too. This
is quite useful if you have lots of passive checks and make use of
nagios-send-gearman/nagios-send-multi where you would have to spread the shared key
to all clients using these tools.
Default is no.
+
====
    accept_clear_results=no
====




Worker Options
~~~~~~~~~~~~~~

Additional options for worker:

identifier::
Identifier for this worker. Will be used for the 'worker_identifier' queue for
status requests. You may want to change it if you are using more than one worker
on a single host.  Defaults to the current hostname.
+
====
    identifier=hostname_test
====


pidfile::
Path to the pidfile.
+
====
    pidfile=/run/nagios-mod-gearman-worker/nagios-mod-gearman-worker.pid
====


job_timeout::
Default job timeout in seconds. Currently this value is only used for
eventhandler. The worker will use the values from the core for host
and service checks.
Default: 60
+
====
    job_timeout=60
====


max-age::
Threshold for discarding too old jobs. When a new job is older than
this amount of seconds it will not be executed and just discarded.
This will result in a message like "(Could Not Start Check In Time)".
Possible reasons for this are time differences between core and
worker (use NTP!) or the smart rescheduler of the core which should be
disabled. Set to zero to disable this check.
Default: 0
+
====
    max-age=600
====


min-worker::
Minimum number of worker processes which should run at any time. Default: 1
+
====
  min-worker=5
====


max-worker::
Maximum number of worker processes which should run at any time. You may set
this equal to min-worker setting to disable dynamic starting of workers. When
setting this to 1, all services from this worker will be executed one after
another. Default: 20
+
====
    max-worker=50
====


spawn-rate::
Defines the rate of spawned worker per second as long as there are jobs
waiting. Default: 1
+
====
    spawn-rate=1
====


load_limit1::
Set a limit based on the 1min load average. When exceeding the load limit,
no new worker will be started until the current load is below the limit.
No limit will be used when set to 0.
Default: no limit
+
====
    load_limit1=0
====


load_limit5::
Set a limit based on the 5min load average. See 'load_limit1' for details.
Default: no limit
+
====
    load_limit5=0
====


load_limit15::
Set a limit based on the 15min load average. See 'load_limit1' for details.
Default: no limit
+
====
    load_limit15=0
====


idle-timeout::
Time in seconds after which an idling worker exits. This parameter
controls how fast your waiting workers will exit if there are no jobs
waiting. Set to 0 to disable the idle timeout. Default: 10
+
====
  idle-timeout=30
====


max-jobs::
Controls the amount of jobs a worker will do before he exits. Use this to
control how fast the amount of workers will go down after high load times.
Disabled when set to 0. Default: 1000
+
====
    max-jobs=1000
====

fork_on_exec::
Use this option to disable an extra fork for each plugin execution.
Disabling this option will reduce the load on the worker host, but may
cause trouble with unclean plugins. Default: no
+
====
    fork_on_exec=yes
====

dupserver::
sets the address of gearman job server where duplicated result will be sent to.
Can be specified more than once to add more server. Useful for duplicating
results for a reporting installation or remote gui.
+
====
    dupserver=logserver:4730,logserver2:4730
====


show_error_output::
Use this option to show stderr output of plugins too. When set to no,
only stdout will be displayed.
Default is yes.
+
====
    show_error_output=yes
====


timeout_return::
Defines the return code for timed out checks. Accepted return codes
are 0 (Ok), 1 (Warning), 2 (Critical) and 3 (Unknown)
Default: 2
+
====
    timeout_return=2
====


dup_results_are_passive::
Use this option to set if the duplicate result send to the 'dupserver'
will be passive or active.
Default is yes (passive).
+
====
    dup_results_are_passive=yes
====


debug-result::
When enabled, the hostname of the executing worker will be put in
front of the plugin output. This may help with debugging your plugin
results.
Default is off.
+
====
    debug-result=no
====


restrict_path::
`restrict_path` allows you to restrict this worker to only execute plugins
from these particular folders. Can be used multiple times to specify more
than one folder.
Note that when this restriction is active, no shell will be spawned and
no shell characters ($&();<>`"'|) are allowed in the command line itself.
+
====
    restrict_path=/usr/local/plugins/
====


workaround_rc_25::
Duplicate jobs from gearmand result sometimes in exit code 25 of
plugins because they are executed twice and get killed because of
using the same ressource. Sending results (when exit code is 25 )
will be skipped with this enabled.
Only needed if you experience problems with plugins exiting with exit
code 25 randomly. Default is off.
+
====
    workaround_rc_25=off
====




Queue Names
-----------

You may want to watch your gearman server job queue. The shipped
nagios-gearman-top does this. It polls the gearman server every second
and displays the current queue statistics.

--------------------------------------
+-----------------------+--------+-------+-------+---------+
| Name                  | Worker | Avail | Queue | Running |
+-----------------------+--------+-------+-------+---------+
| check_results         | 1      | 1     | 0     | 0       |
| host                  | 3      | 3     | 0     | 0       |
| service               | 3      | 3     | 0     | 0       |
| eventhandler          | 3      | 3     | 0     | 0       |
| servicegroup_jmx4perl | 3      | 3     | 0     | 0       |
| hostgroup_japan       | 3      | 3     | 0     | 0       |
+-----------------------+--------+-------+-------+---------+
--------------------------------------


check_results::
this queue is monitored by the neb module to fetch results from the
worker. You don't need an extra worker for this queue. The number of
result workers can be set to a maximum of 256, but usually one is
enough. One worker is capable of processing several thousand results
per second.


host::
This is the queue for generic host checks. If you enable host checks
with the hosts=yes switch. Before a host goes into this queue, it is
checked if any of the local groups matches or a separate hostgroup
matches. If nothing matches, then this queue is used.


service::
This is the queue for generic service checks. If you enable service
checks with the `services=yes` switch. Before a service goes into this
queue it is checked against the local host- and service-groups. Then
the normal host- and servicegroups are checked and if none matches,
this queue is used.


hostgroup_<name>::
This queue is created for every hostgroup which has been defined by
the hostgroups=... option. Make sure you have at least one worker for
every hostgroup you specify. Start the worker with `--hostgroups=...`
to work on hostgroup queues. Note that this queue may also contain
service checks if the hostgroup of a service matches.


servicegroup_<name>::
This queue is created for every servicegroup which has been defined by
the `servicegroup=...` option.


eventhandler::
This is the generic queue for all eventhandler. Make sure you have a
worker for this queue if you have eventhandler enabled. Start the
worker with `--events` to work on this queue.


perfdata::
This is the generic queue for all performance data. It is created and
used if you switch on `--perfdata=yes`. Performance data cannot be
processed by the gearman worker itself. You will need
http://www.pnp4nagios.org[PNP4Nagios] therefor.


Performance
-----------

While the main motivation was to ease distributed configuration, this
application also helps to spread the load on multiple worker. Throughput is
mainly limited by the amount of jobs a single Nagios instance can put
onto the Gearman job server. Keep the Gearman job server close to the
Nagios box. Best practice is to put both on the same machine. Both
processes will utilize one core.
The amount of worker boxes required depends on your check types.


Exports
-------
Exports export data structures from the Nagios core as JSON data. For
each configurable event one job will be created. At the moment, the
only useful event type is the logdata event which allows you to create
a json data job for every logged line. This can be very useful for
external reporting tools.

exports::
Set the queue name to create the jobs in. The return code will be sent
back to the core (Not all callbacks support return codes). Callbacks
are a list of callbacks for which you want to export json data.
+
====
    export=<queue>:<returncode>:<callback>[,<callback>,...]

    export=log_queue:1:NEBCALLBACK_LOG_DATA
====



How To
------

How to Monitor Job Server and Worker
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Use the supplied nagios-check-gearman to monitor your worker and job server.
Worker have a own queue for status requests.

--------------------------------------
%> nagios-check-gearman -H <job server hostname> -q worker_<worker hostname> -t 10 -s check
nagios-check-gearman OK - localhost has 10 worker and is working on 1 jobs|worker=10 running=1 total_jobs_done=1508
--------------------------------------

This will send a test job to the given job server and the worker will
respond with some statistical data.

Job server can be monitored with:

--------------------------------------
%> nagios-check-gearman -H localhost -t 20
nagios-check-gearman OK - 6 jobs running and 0 jobs waiting.|check_results=0;0;1;10;100 host=0;0;9;10;100 service=0;6;9;10;100
--------------------------------------



How to Submit Passive Checks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can use nagios-send-gearman to submit active and passive checks to a
gearman job server where they will be processed just like a finished
check would do.

--------------------------------------
%> nagios-send-gearman --server=<job server> --encryption=<yes|no> --key=<string> --host="<hostname>" --service="<service>" --message="message"
--------------------------------------



How to Set Queue by Custom Variable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Set 'queue_custom_variable=worker' in your Nagios-Mod-Gearman NEB
configuration. Then adjust your Nagios host/service configuration and
add the custom variable:

-------
  define host {
    ...
    _WORKER    hostgroup_test
  }
-------

The test hostgroup does not have to exist, it is a virtual queue name
which is used by the worker.

Adjust your Nagios-Mod-Gearman worker configuration and put 'test' in the
'hostgroups' attribute. From then on, the worker will work on all jobs
in the 'hostgroup_test' queue.


Notifications
-------------
Nagios-Mod-Gearman does distribute Notifications as well.
All Notifications (except bypassed by local groups) are send into the notifications
queue and processed from there.

Nagios-Mod-Gearman does not support environment macros, except two plugin output related
ones.

It does set

    - NAGIOS_SERVICEOUTPUT
    - NAGIOS_LONGSERVICEOUTPUT

for service notifications and

    - NAGIOS_HOSTOUTPUT
    - NAGIOS_LONGHOSTOUTPUT

for host notifications.


Supported Dependencies
----------------------

Nagios
~~~~~~
Nagios-Mod-Gearman works with Nagios 4.5.x or greater.

 * https://www.nagios.org/projects/nagios-mod-gearman/[Nagios]



Hints
-----
 - Make sure you have at least one worker for every queue. You should
   monitor that (nagios-check-gearman).
 - Add Logfile checks for your gearmand server and nagios-mod-gearman
   worker.
 - Make sure all gearman checks are in local groups. Gearman self
   checks should not be monitored through gearman.
 - Checks which write directly to the Nagios command file (ex.:
   nagios.cmd) have to run on a local worker or have to be excluded by
   the localservicegroups.
 - Keep the gearmand server close to Nagios for better performance.
 - If you have some checks which should not run parallel, just setup a
   single worker with --max-worker=1 and they will be executed one
   after another. For example for cpu intensive checks with selenium.
 - Make sure all your worker have the Nagios-Plugins available under
   the same path. Otherwise they couldn't be found by the worker.
