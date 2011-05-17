===============
NOX Box
===============

Overview
========

.. image:: noxbox_small.jpg
  :align: right

The NOX Box is a small form-factor PC running OpenFlow and NOX on top of
a fairly standard Debian GNU/Linux installation. It offers a simple, inexpensive platform for anyone
looking to experiment with NOX and OpenFlow. In addition, the NOX Box comes with
tools to configure NAT, PPPoE, DNS, DHCP, and wireless access point software,
making it easy to install as a flexible Internet gateway for the
home or small office.

The software for the NOX Box is freely available as a 1 GB
`binary image <http://noxrepo.org/current_noxbox.img.bz2>`_, which you
can copy to a Compact Flash card that acts as the device's persistent storage.
We currently use the `PC Engines ALIX 2c3 <http://www.pcengines.ch/alix2c3.htm>`_
board as our hardware platform, along with a few other commodity components (an Atheros Mini PCI wireless card, antennas, etc.).
See our `Hardware Guide`_ for details on how you can build your own NOX Box.

This manual assumes some familiarity with NOX and OpenFlow. Please refer to
the `NOX Manual <http://noxrepo.org/manual/>`_ for further information.


Quick Start
===========

The basic steps to getting a NOX Box up and running are:

1. Buy the hardware components need to build a NOX Box, as described in the `Hardware Guide`_.
2. Download the `NOX Box software image <http://noxrepo.org/current_noxbox.img.bz2>`__.
3. Copy the image to the Compact Flash card, as described in `Preparing the Compact Flash Card`_.
4. Assemble the hardware and boot the system by plugging it in.
5. Connect to the NOX Box by associating with the SSID "NOX Box" (the WPA passphrase is "n0xb0xr0x") or by plugging a cable into Port 2 or Port 3 (see `Ethernet Ports`_). Your machine should receive an IP address in the 10.100.0.* range via DHCP.
6. If you plan on using the NOX Box as an Internet gateway, connect Port 1 (the one nearest the power connector) to your cable/DSL modem or other upstream connection.
7. Configure the correct mode for `Ethernet Port 1`_. If you want to SSH in and do this by hand, go on to step 8. If you want to do this from the front panel:

   (1) Use a paper clip with the front panel button to set the correct mode for the upstream link (e.g., NAT or PPPoE). See `Changing the Port 1 Mode`_.
   (2) If using PPPoE, you will want to read the `Configuring PPPoE`_ and `PPPoE Learning`_ sections.

8. Restart your cable/DSL modem if appropriate.

You can SSH into the gateway IP of 10.100.0.1 with the username "root" and
password "n0xb0xr0x".



NOX Box Basics
==============

This section will help you get to know your NOX Box.


The Front Panel
---------------

The front panel of the NOX Box has three LED indicator lights and a pushbutton that
can be pressed with a paper clip.

The indicator lights can be used to gather information about the NOX Box's status during boot/shutdown and various other state information when it is running. Their exact meanings are detailed in the two following sections.

The pushbutton can be used to make certain basic configuration changes, safely restart/shut down the NOX Box, and "rescue" a box which has been rendered unreachable (e.g., due to misconfiguration or a faulty NOX application). Its operation is detailed in the `Configuration via the Front Panel`_ section.


System Status Indication
------------------------

During normal operation, the status LEDs on the front panel of the NOX Box
describe the current running state. The LEDs indicate the following conditions (from
left to right):

.. list-table:: LED Status Lights
   :widths: 7 20 60
   :header-rows: 1

   * - LED
     - Name
     - Description
   * - Left
     - NOX Box Ready
     - Lit when the NOX Box is fully booted
   * - Center
     - NOX Up
     - Lit when NOX is running
   * - Right
     - Uplink Status
     - Blinking slowly when attempting to connect by PPP. Lit when PPP is up or if Port 1 has an IP address (e.g. as gotten from DHCP when in NAT mode).


Powering the NOX Box On and Off
-------------------------------

Starting up the NOX Box is as simple as plugging it in, and -- when running in Production Mode (see `Production Mode and Developer Mode`_) -- it should be okay to just pull the power on the NOX Box to shut it down. However, a downside to simply pulling the plug is that any log files which have not yet been saved to the Compact Flash (they are only saved occasionally) will be lost. If these log files are important to you, you will want to shut the NOX Box down safely. You will also wish to shut the NOX Box down safely if you are running in Developer Mode.

Reading the Front Panel Indicators During Startup and Shutdown
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the initial stages of booting, the leftmost LED will be on and the others
will be off. Part way through booting, the leftmost LED will turn off. When LEDs light up again, they will be displaying status information as detailed in the `System Status Indication`_ section.

Just before the system shuts down or restarts, all the lights will come on and
will then "count down" by turning off one at a time, starting from the right.

Safely Shutting Down
^^^^^^^^^^^^^^^^^^^^

In order to shut the NOX Box down, you can simply SSH in and use the "shutdown" command as usual (i.e. "shutdown -h now"). The NOX Box does not have the ability to actually turn itself off, but after the lights have all gone off (after performing the distinctive "count down" sequence as described in the previous section), you can wait three seconds and then pull the plug.

You may also shut the box down by pressing and holding the front panel button with a paper clip until all three lights are blinking rapidly, and then releasing the button. This causes the NOX Box to restart. Again, you simply watch for the LEDs to perform the "count down" sequence, wait three seconds, and pull the plug. See the `Other Front Panel Operations`_ section for more information on using the front panel.


Ethernet Ports
--------------

.. image:: ports.jpg
  :align: right

NOX Boxes generally have at least two Ethernet ports.

**Note**: Ports are numbered from **right to left** as seen from the back, starting at 1.

All the ports can be added to the OpenFlow switch, and ports 1 and 2 can also be assigned various special purposes. The port modes are set based on settings in `The Configuration File`_, and also by `The Front Panel`_ if it is configured (it is by default). There are also scripts (e.g. port1_nat, port2_debug) to make immediate, non-persistent changes to a port status.


Ethernet Port 1
^^^^^^^^^^^^^^^

If your NOX Box will act as an Internet gateway, Port 1 acts as the the "upstream" port, and can be configured to run NAT (needed for cable services) or NAT with PPPoE (needed for many DSL services). Of course, if your NOX Box is not acting as a gateway, Port 1 can also just act as another port on the OpenFlow switch.

.. list-table:: Port 1 Modes
    :widths: 12 60
    :header-rows: 1

    * - Mode Name
      - Description

    * - **off** [#front-panel-configurable]_
      - This mode is just like *pppoe* and *nat* modes (see below), but the "uplink" (Port 1) is disabled.

        As in pppoe and nat modes, the other interfaces share a common address as specified by IP_ADDR in
        `The Configuration File`_, and DNS and DHCP are enabled unless overridden by DNS_AND_DHCP.

        The NOX Box comes configured in this mode.

    * - **on** [#front-panel-configurable]_
      - In this mode, Port 1 is added to the OpenFlow switch, and the NOX Box has a single address, which is controlled by the ON_IP_ADDR configuration parameter (see `The Configuration File`_).

        DNS and DHCP are disabled unless overridden by DNS_AND_DHCP.

    * - **nat** [#front-panel-configurable]_/ **pppoe** [#front-panel-configurable]_
      - These two modes are similar in that the NOX Box has two addresses --
        Port 1 has an "outside" address provided via PPP or DHCP (for pppoe and
        nat modes respectively), and the other interfaces share an "inside" address which is
        controlled by the IP_ADDR configuration parameter (see: `The Configuration File`_).

        DNS and DHCP are enabled unless overridden by DNS_AND_DHCP.

    * - **dhcp**
      - In this mode, Port 1 gets an address via DHCP. It is not joined to the OpenFlow switch. This is useful when Port 1 will be used for out-of-band communication with an external controller.

        DNS and DHCP are disabled unless overridden by DNS_AND_DHCP.

    * - **none**
      - This isn't really a mode at all. This just instructs the NOX Box to leave Port 1 entirely alone. This way, you are free to configure it in a site-specific way (in rc.local or /etc/network/interfaces, for example).



.. [#front-panel-configurable] This mode can be set from the front panel as detailed in the `Changing the Port 1 mode`_ section.


Ethernet Port 2
^^^^^^^^^^^^^^^^

Port 2 is special in that it can easily be put into a "debug mode", in which it is taken off of the OpenFlow switch and simply given a known IP address. This can be done from the front panel (see `Other Front Panel Operations`_) and provides a last resort for gaining access to a box which has been misconfigured.

.. list-table:: Port 2 Modes
    :widths: 12 60
    :header-rows: 1

    * - Mode Name
      - Description

    * - **off**
      - Port 2 is disabled.
    * - **on**
      - In this mode, Port 2 is simply added to the OpenFlow switch.
    * - **debug**
      - Port 2 is simply assigned the address from PORT2_IP_ADDR
        (see `The Configuration File`_).  Note that you must configure an IP
        address for your machine by hand -- the NOX Box will *not* serve you an
        appropriate one by DHCP.
    * - **none**
      - This isn't really a mode at all. This just instructs the NOX Box to leave Port 2 entirely alone. This way, you are free to configure it in a site-specific way (in rc.local or /etc/network/interfaces, for example).

Port 2 can be toggled between its setting in `The Configuration File`_ and debug mode from the front panel. The procedure is detailed in the `Other Front Panel Operations`_ section.


Configuration
=============

For simple home router type setups, you may well be able to configure the NOX
Box entirely from the front panel. Other configurations may require changing
configuration files by hand.

If you find that what you want to do is not easily supported, please let us
know.


Configuration via the Front Panel
---------------------------------

For many common scenarios, the NOX Box can be configured with a paper clip and the front panel pushbutton.

For more information on the front panel (including how to read the LEDs), please refer to the `NOX Box Basics`_ section.


Changing the Port 1 mode
^^^^^^^^^^^^^^^^^^^^^^^^

The Port 1 mode (see `Ethernet Ports`_) can be inspected or changed from the
front panel.

To see the current Port 1 mode, press and release the button. This will put the panel in Port 1 Mode Select Mode. The lights will flash, and then the Port 1 mode will be reported as follows:

.. list-table:: Port 1 LED Indicators
   :widths: 15 15
   :header-rows: 1

   * - LEDs
     - Port 1 Mode

   * - 000
     - off
   * - 100
     - on
   * - 010
     - nat
   * - 001
     - pppoe

After a few moments, the lights will blink three times, and will then go back
to monitoring status.

If you wish to change the Port 1 mode, press the button to enter Port 1 Mode Select Mode. Then press the button several times in succession until the lights indicate the mode you would like to become
active. When the appropriate mode is indicated, just wait for the lights
to blink again, indicating the new mode has been selected.


PPPoE Learning
^^^^^^^^^^^^^^

If you are using the NOX Box to connect via PPPoE (as many DSL providers do, for
example), the NOX Box can attempt to "learn" PPPoE configuration from another
device which has already been configured. For this to work, your ISP must be
using PPP Password Authentication Protocol [RFC1334]_.

 * Plug your NOX Box's Port 1 into your DSL modem
 * Press the button on the NOX Box to go into Port 1 Mode Select Mode (see `Changing the Port 1 Mode`_)
 * Select PPPoE mode (rightmost LED blinking) if it is not already selected (again, see `Changing the Port 1 Mode`_)
 * Press and hold the button until the LEDs begin "scanning" back and forth
 * Plug your other device (i.e. your old router or computer) in to Port 2
 * Initiate a PPPoE connection from your other device (in many cases you need not do anything -- the device will already be trying)
 * Wait for the LEDs to blink
 * Disconnect your other device

The NOX Box will wait for up to two minutes looking for PPPoE login information. If the process completes successfully, the NOX Box LEDs will blink six times and a PPPoE connection will be initiated (this can take a minute or so). If the process does not complete successfully, the LEDs will blink three times, and you will have to configure PPPoE by hand (see `Configuring PPPoE`_). In either case, the panel will return to monitoring status.


Other Front Panel Operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When you hold the button down, the LEDs will blink rapidly in various combinations that represent a number of options. When the blinking combination of LEDs represents your desired action (as indicated in the table below), release the button. If you do not want to perform any of the actions, just keep holding the button down until the lights go back to status monitoring.


.. list-table:: LED / Pushbutton Combination Actions
   :widths: 10 60
   :header-rows: 1

   * - LEDs
     - Action

   * - 100
     - Cycle NOX back to DEFAULT_NOX (see `The Configuration File`_)
   * - 010
     - Toggle `Ethernet Port 2`_ between "debug mode" and the behavior specified by PORT2_MODE in `The Configuration File`_
   * - 110
     - Shut down NOX (this will cause the switch to fail open in a short time)
   * - 111
     - Restart the NOX Box (also useful for executing a graceful shutdown as described in `Safely Shutting Down`_)


Configuring PPPoE
-----------------

``configpppoe <username> <password>``

The configpppoe script lets you easily set the PPPoE username and password from the commandline.

The NOX Box can also attempt to learn your PPPoE configuration from an existing device. See the `PPPoE Learning`_ section.


Configuring WiFi
----------------

.. warning::
  The wireless configuration is not as ironed out as some aspects of the NOX Box
  and is likely to change for the next release.

To change wireless security modes (i.e. to WPA2) or to change the SSID, you
will want to edit the configuration file for hostapd. This is stored in
/etc/hostapd/hostapd.conf.

If you wish have an open access point or use WEP, you will want to stop
hostapd from running at all!  Disable it using /etc/default/hostapd. You
will also probably want to change the configuration for ath0 in
/etc/network/interfaces.


Configuring the WPA Passphrase
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``configwpa <passphrase>``

The configwpa script lets you set the WPA passphrase easily from the commandline.



Adding Software to the NOX Box
------------------------------

A key benefit of the NOX Box is that it runs a fairly standard installation of
Debian GNU/Linux, not a special
distribution meant for embedded devices. As a result, you can easily install
new software packages using apt-get (assuming your configuration has a working
network connection).

In general, you should only install software while in Developer Mode (see
`Developer Mode in Detail`_). When in Production Mode, software will be
installed to a RAM disk. This impacts software installation in two significant
ways: there is not much space available, and changes will not be persistent.


The Configuration File
----------------------

The NOX Box's main configuration file is /etc/noxbox.conf. It controls many
aspects of the NOX Box's operation. *However*, if you are running the front
panel software, you also may wish to read about its configuration file in the
`Configuring the Front Panel`_ section. In particular, this effects
the PORT1_MODE parameter.

.. list-table:: noxbox.conf parameters
  :widths: 10 60 10 10
  :header-rows: 1

  * - Name
    - Meaning
    - Values
    - Default

  * - PORT1_MODE
    - This controls which Port 1 mode (see `Ethernet Port 1`_) the NOX Box
      configures at boot time.

       **Note**: If you are using the front panel software (which we enable by
       default), you should generally leave this as "none", and let the
       front panel software manage it.
    - Port 1 mode
    - off

  * - PORT2_MODE
    - This controls which Port 2 mode (see `Ethernet Port 2`_) the NOX Box
      configures at boot time.
    - Port 2 mode
    - on

  * - IP_ADDR
    - This controls the IP address that the NOX Box will use for its "inside"
      network when Port 1 is in PPPoE, NAT, or "off" modes. [#IP]_
    - IPv4 Address
    - 10.100.0.1

  * - ON_IP_ADDR
    - This controls the IP address that the NOX Box will use for itself when
      Port 1 is in "on" mode.
    - IPv4 Address or "DHCP"
    - DHCP

  * - PORT2_IP_ADDR
    - Specifies the address to be used when Port 2 is in debug mode.
    - IPv4 Address
    - 172.16.172.16

  * - PORT2_NETMASK
    - Specifies the netmask to be used when Port 2 is in debug mode.
    - Netmask
    - Based on address

  * - DNS_AND_DHCP
    - For each Port 1 mode, the NOX Box may default to running a DNS and DHCP
      server or not (in general, the Internet gateway type modes will, the
      others will not). This parameter will override the default.
    - Boolean or "default".
    - default

  * - CONTROLLER_ADDR
    - The address at which the OpenFlow switch (secchan, really) expects to
      find a controller.
    - Address[:Port]
    - localhost

  * - OPENFLOW_MAC
    - By default, OpenFlow uses an arbitrary MAC for the of0 interface. If
      you require a fixed or specific MAC address, this
      parameter lets you specify one (e.g.
      "d2:78:ed:e2:f3:d7").
    - MAC Address
    - Random

  * - OPENFLOW_INTERFACES
    - Specifies which interfaces will be added to the OpenFlow switch. You
      do not need to add eth0 and eth1 here, as those interfaces are handled
      by the PORT1_MODE and PORT2_MODE parameters above (i.e. just set those
      to "on" if you want them on the OpenFlow switch).

       **Note**: For multiple interfaces, the list should be surrounded by
       quotes and items should be separated by spaces.
    - Quoted list of interfaces
    - "" (None)

  * - DEFAULT_NOX
    - The commandline to use to invoke a NOX controller. If empty, NOX will
      not be run automatically.

       **Note**: You should enclose this in quotes, and be sure to include
       the leading "./".
    - Commandline
    - None

  * - DEFAULT_NOX_DIR
    - The directory in which the NOX commandline specified by DEFAULT_NOX will
      be executed.
    - Path
    - /noxbox/nox/src

  * - MAGIC_AP_BRIDGE
    - Normal switch procedures will not allow communication between two
      wireless stations on the same access point. To rectify this, Linux
      wireless drivers implement an internal bridge. Enabling this internal
      bridge allows wireless stations to communicate, but since this bridge
      exists within the driver, such communication can not be controlled or
      monitored by OpenFlow and NOX. A future release of NOX will remedy this
      situation.

      A "true" value will turn on the internal bridge and allow wireless
      stations to communicate with each other.
    - Boolean
    - false

.. [#IP] Please note the following warning.

.. warning::
  Changing the IP address does *not* currently reconfigure the DHCP and DNS
  server. If you use DNS and DHCP, you will want to reconfigure the server
  by hand by editing /etc/dnsmasq.conf. This is likely to change in the
  next release.


Configuring the Front Panel
-----------------------------

The panel control software reads configuration information from /etc/panel.conf. You may wish to adjust this file to suit your specific installation.

For information on customizing the front panel, see `Hacking the Front Panel`_.

.. list-table:: panel.conf parameters
  :widths: 10 60 10 10
  :header-rows: 1

  * - Name
    - Meaning
    - Values
    - Default

  * - CONFIGURE_PORT1
    - Determines whether the panel will attempt to manage the Port 1 mode. If
      you *do* want the panel to manage the Port 1 mode, you should configure
      the main NOX Box configuration file (see `The Configuration File`_) to
      leave Port 1 alone.
    - Boolean
    - false

  * - PORT1_MODE
    - Similar to the parameter of the same name in the main NOX Box
      configuration file (see `The Configuration File`_). Note that panel.py
      will update this value!

      Allowable values are any Port 1 mode name (though, really, you should
      probably never configure this by hand, and just leave it to panel.py).
    - Port 1 mode
    - off

  * - BUTTON_ENABLED
    - Determines whether panel.py will attempt to read the pushbutton state.
      If your hardware does not have a pushbutton, you may need to disable
      this.
    - Boolean
    - false













Inside the NOX Box
==================

This section describes some of the internals of the NOX Box software
environment. If you wish to configure the NOX Box in some unsupported way or
plan to run your own code on the NOX Box, this section provides some background
information about the NOX Box's workings which will benefit you.

If you find you still have questions, feel free to join us on `the NOX
development mailing list
<http://mail.noxrepo.org/mailman/listinfo/nox-dev_noxrepo.org>`_.


Monitoring NOX and secchan
--------------------------

The NOX Box automatically starts nox_core (if NOX has been configured in
`The Configuration File`_) and secchan within a detatched screen (if you are
unfamiliar with screen, see `this tutorial
<http://www.kuro5hin.org/print/2004/3/9/16838/14935>`_). After SSHing in to
the NOX Box, you can connect to this screen with the command "screen -r".

Within the screen, there are separate windows for NOX, secchan, and the front
panel controller (this last window is mainly for debugging and can usually be
ignored).

You can detach from a screen using Control-A followed by "d".


The Filesystem
--------------

This section describes the setup of the NOX Box filesystem -- the directory
layout of the NOX Box software, the where/why/how of mounted disks, and
information on making your changes and customizations "stick" across reboots.


Layout
^^^^^^

Most key files are located within the /noxbox/ directory. The main exception to this is that `The Configuration File`_ and configuration files from other packages (e.g. hostapd, the program which provides WPA wireless encryption) are in their usual locations (e.g., /etc/).

/noxbox/ includes the following subdirectories:

 * **bin**: Utilities for reconfiguring and manipulating the NOX Box. These are intended for external use and are already in your $PATH.
 * **nox**: The current NOX code and binaries.
 * **openflow**: The current OpenFlow code and binaries.
 * **panel**: Internal code for controlling the front panel. You can ignore this unless you want or need to customize the front panel.
 * **admin**: Internal scripts and binaries used by other NOX Box scripts. You can ignore this unless you are changing the behavior of the NOX Box.
 * **init**: The NOX Box init.d scripts. Links in /etc/init.d/ point to files in here. You can ignore this unless you are changing the behavior of the NOX Box.

Production Mode and Developer Mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The NOX Box is intended to fill at least two roles: that of a replacement to a common
home or small office wireless router, and that of a small development platform
for NOX and OpenFlow. The expectations and requirements for these two roles
are somewhat different. For example, in the former role, the box is expected
to "just work" even when the standard method for turning it off is simply pulling
the plug. In the latter role,it is expected that the system be easy to install
software on, reconfigure, etc. To best support these roles, the NOX Box has
two major modes of operation: Production Mode and Developer Mode.


Production Mode in Detail
+++++++++++++++++++++++++

In Production mode, the NOX Box is configured to minimize writing to the
Compact Flash card significantly by using `aufs <http://aufs.sourceforge.net/aufs.html>`_ to overlay a
RAM disk on top of the Compact Flash card's filesystem. The actual Compact
Flash card is mounted read-only (writes go to RAM), which means that a sudden
loss of power should not leave the filesystem in an inconsistent state -- when
powered up again, it should be just as it was before. Compact Flash cards only
support a finite number of writes before they begin to fail, so this also helps
prevent "wear".

The filesystem mounted at / (the root filesystem) contains a merged view of the Compact Flash
card and a RAM disk. The two underlying filesystems are also mounted individually at /ro/ and /rw/ respectively.



Developer Mode in Detail
++++++++++++++++++++++++

The Developer mode configuration is more like a normal desktop workstation
configuration -- the Compact Flash card is simply mounted read/write at /,
though areas that contain frequently written and transient data (/tmp/, /var/tmp/, /var/lock/,
/var/run/, and /var/log/) are RAM disks.

In this mode, the system really should be shut down safely to avoid
leaving the filesystem in an inconsistent state (see `Safely Shutting Down`_).
Developer mode results in more writes to the Compact Flash card than Production
Mode, potentially reducing the card's lifespan.


Switching Between Production and Developer Modes
++++++++++++++++++++++++++++++++++++++++++++++++

You can only switch between production and
developer modes by rebooting. This is done with the NOX Box "devmode" script. "devmode
on" will cause the NOX Box to boot up in developer mode until "devmode off" is
run. Without arguments, the devmode script shows the current mode and the mode
upon reboot.

If connecting by serial cable, you may also force the NOX Box
into developer mode for a single session with a boot menu option.


Saving Changes Made While In Production Mode
++++++++++++++++++++++++++++++++++++++++++++

Since writes done while in Production Mode go to a RAM disk, they are lost
when the device loses power. If you do make changes that you wish to keep,
you must somehow copy the changed files from the RAM disk to the Compact Flash.
There are several ways to go about this, and there are a number of scripts to
help you.

**Tip**: You can easily see all files that have changed by using "find /rw".

.. list-table:: Persistence Tools
   :widths: 10 60
   :header-rows: 0

   * - **devmode**
     - Run with no arguments, displays the current mode and the mode upon reboot.
       Run with arguments, changes the mode upon reboot: "devmode on" enables developer mode, "devmode off" disables.

   * - **remountrw** / **remountro**
     - When run from Production mode, remounts the Compact Flash as read/write
       or read-only. When it is mounted as read/write, you can simply edit
       files on it, or copy files to it from the RAM disk (/rw/).

   * - **persist_files**
     - When run from Production mode, persists one or more files to the Compact Flash.

   * - **persist_changes**
     - When run from Production mode, persists all changed files to the Compact Flash. This includes *all* files, including some which it is not generally desirable to save (e.g. lock files).

   * - **persist_logs**
     - As mentioned in the `Log Files`_ section, this script persists logs. It
       is run automatically from a cron job and when shutting down, and is not generally interesting
       to users in and of itself.

If you wish to write your own scripts that persist data, the file
/noxbox/admin/persistence_tool_base.sh contains some functions for doing so.
You also may well wish to start by modifying an existing tool, such as
persist_files (which makes use of persistence_tool_base.sh).


Log Files
---------

As discussed in the `Production Mode and Developer Mode`_ sections, /var/log/
is always a RAM disk. In order to save logs, a cron job periodically runs
the persist_logs program to copy log files to the Compact Flash card. This
program also compresses saved logs and shifts old ones out in a manner similar
to the logrotate utility.

Currently, only the messages log is persisted in this way. persist_logs was
written with adding more logs in mind, though it is not currently implemented.
It should be easy to add.


The init.d Scripts
------------------

Much of the NOX Box's behavior is controlled by run control scripts. In many
cases, exactly what these scripts do is determined by parameters in `The
Configuration File`_.

As with any init script, you can invoke these all with "invoke-rc.d" to start,
stop, and restart their services (and reload/realize settings).

.. list-table:: NOX Box init.d scripts
    :widths: 10 60
    :header-rows: 0

    * - nox
      - Starts and stops NOX if DEFAULT_NOX is configured.

        The started NOX runs via the nox_loop.sh script in a detached "screen"
        session where it can be monitored. See `Monitoring NOX and secchan`_.

    * - noxboxready
      - Sets and unsets a flag in the filesystem that indicates if the NOX Box
        is "ready" or not (this is what controls the leftmost LED on the front
        panel, for example).

    * - openflow
      - At startup, initializes and configures OpenFlow. This includes setting
        the MAC address for of0 if OPENFLOW_MAC was specified, adding the
        interfaces specified by OPENFLOW_INTERFACES to the switch, and
        starting secchan to connect to a controller at CONTROLLER_ADDR.

        As with "nox" and "panel", the started instance of secchan runs in a
        detached "screen" session where it can be monitored. See `Monitoring
        NOX and secchan`_.

        At shutdown, deconfigures OpenFlow.

    * - panel
      - Starts and stops the front panel control program.

    * - panel_goodbye
      - Displays the "countdown" sequence on the front panel when the system
        shuts down.

    * - persist_startup_shutdown
      - Does various things concerning persistence. For example, making sure
        that logs are persisted when shutting down.

    * - port1
      - Configures the Port 1 mode as per PORT1_MODE.

    * - port2
      - Configures the Port 1 mode as per PORT2_MODE.



Hacking on the NOX Box
======================

While the previous section focused on how the NOX Box is set up, this one
focuses on what it takes to actual develop software on and for the NOX Box.

This section assumes you are familiar with the information in `Inside the NOX
Box`_. If you haven't read it, you may wish to go back and do so now. Most
importantly, you should be familiar with `Production
Mode and Developer Mode`_.


Writing NOX Applications
------------------------

General information about developing for NOX is found in the
`NOX Manual <http://noxrepo.org/manual/>`_. The specifics of developing for
the NOX Box are really just understanding the limitations of the platform, the
difficulties of developing networking software when your interface to the
system is over the network, and understanding a bit about the environment
in which NOX will be running and the tools available to you.

The cyclenox Tool
^^^^^^^^^^^^^^^^^

As previously mentioned, the NOX Box automatically runs NOX using the
commandline given as DEFAULT_NOX in the directory given by DEFAULT_NOX_DIR
from `The Configuration File`_. When you're testing new NOX applications (or
new versions of NOX), you will be wanting to run using a different commandline
or from a different directory.

The cyclenox utility facilitates this. cyclenox can be given an alternate
commandline (with a directory if desired), and it will restart NOX using
that commandline -- *once*. The *next* time NOX starts, it will again be
started using the settings specified in noxbox.conf. This way, if NOX fails
to start up, you'll immediately get back to the default configuration.
Similarly, if your new commandline accidentally locks you out of your NOX Box,
you can use the front panel to get you back (as specified in
`Other Front Panel Operations`_).

For more information on using this tool, please see "cyclenox --help".



Building Software on a Desktop PC
---------------------------------

While it is reasonable to re-compile small changes to NOX or OpenFlow on the
NOX Box, doing a compile of the entire code bases will take a *long* time.
A more reasonable model (the one we use for the versions of OpenFlow and NOX
for the NOX Box) is to build software on a fast workstation, and then just
transfer them over with scp.

There is no real trick to this. The NOX Box is a fairly standard Debian i386
installation running Debian testing or Debian unstable with kernel 2.6.24.
If you set up a PC to run this type of system, you can simply compile on it and
then move the binaries to the NOX Box. This often also involves using apt-get
to fetch dependencies, but is generally quite easy.

One easy way to get such a system is to use a NOX Box image as the basis for
a virtual machine hard drive image that you run on your desktop system. This
way you are sure to be compiling in the same environment. Your setup is not as
efficient as it might be, but it still may well be easier than developing
directly on the NOX Box.


Building NOX and OpenFlow for the NOX Box
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Essentially, you can just build NOX and OpenFlow on another machine with a
similar setup and bring the binaries over to the NOX Box. However, doing a
couple simple things will probably make your life easier.

Build OpenFlow and (especially) NOX in a directory which mimics their placement
on the NOX Box (i.e. in /noxbox/openflow/ and /noxbox/nox/). If you don't
have C++ installed on your NOX Box, you should run nox (nox_core) once on the
build machine before moving it to the NOX Box. These steps will smooth over
some issues with symlinks and libtool.


Hacking the Front Panel
-----------------------

`The Front Panel`_ makes a number of assumptions about your hardware and configuration. For example, it assumes you have at least two Ethernet ports, that you want to be running in one of four Port 1 modes, that your front panel has three LEDs and a pushbutton, etc. It also assumes that you want to be using it!  If these assumptions do not hold, you may wish to disable or modify the panel's operation. If what you cannot achieve what you want by changing the panel's configuration file as described in `Configuring the Front Panel`_, you can modify or replace the control software.

If you want to modify the existing software (which has some good points and some bad points and some very messy code with little documentation), it is in /noxbox/panel/daemon/. panel.py is the main program.

If you wish to write your own panel software, it's fairly simple. A kernel module reads provides three files in the /sys/ pseudo-filesystem: /sys/class/leds/alix:1/brightness, /sys/class/leds/alix:2/brightness, and /sys/class/leds/alix:3/brightness. Writing a "1" to any of these will turn the corresponding LED on. Writing a "0" will turn it off. The state of the pushbutton can be read from /proc/alixbutton. A "1" indicates that the button is down. A "0" indicates that it is up.

Preparing the Compact Flash Card
================================

The NOX Box software is distributed as a complete raw image of a Compact Flash
card, which has been compressed with bzip2. This image file can be transferred to a Compact Flash card by using a computer equipped with a "Compact Flash reader" (although this is the common term for such devices, it is a misnomer -- they allow writing too!).  The process of actually writing the image to the card
will probably take between 10 and 20 minutes depending on the speed of the
card reader and the card itself.

The first step to preparing a
Compact Flash card for use in a NOX Box is acqiring the image file.  The most
recent version can always be gotten from:

  `<http://noxrepo.org/current_noxbox.img.bz2>`_

Once you have the Compact Flash image, there are many ways to go about actually
getting it on to a Compact Flash card.  The method we describe can be made
to work under recent versions of Windows, MacOS X, and Linux (and probably
other OSes as well).

Under all three of these operating systems, the most difficult part is
identifying the name that the OS has assigned to the block device associated
with your Compact Flash card (the "disk name"). For help on finding this,
see the appropriate OS-specific section below.

After finding the device name, you can use the bzip2 and dd utilities to decompress and
write the image file to the Compact Flash card using a commandline similar
to the following (be sure to change "<device name>" to the actual device name
you found, i.e. "/dev/sdb")::

  bunzip2 current_noxbox.img.bz2 | dd if=- of=<device name>

**Note**: This step may need to be done as a privileged user (i.e. root or
an Administrator).


Linux-Specific Tips
-------------------

The disk name will probably be "/dev/sd?", where ? is a lower case letter --
something like "/dev/sdb".  Individual partitions on this disk will have a number following the disk name (i.e., "/dev/sdb1" for the first partition).

Some devices have hardware ID values that indicate the maker of the Compact
Flash card, in which case running "ls -l /dev/disk/by-id" can help you figure
out which device you are interested in.  Viewing "/proc/partitions" can also supply useful information.

Assuming that your Compact Flash card is partitioned (it usually is), one way of finding the
card is by listing partitions, inserting the card, listing partitions again,
and noting which paritions are new.  Begin with no card in the card reader.  Then::

  # ls /dev/sd??
  ls: cannot access /dev/sd??: No such file or directory

Now insert the card, and look for partitions again::

  # ls /dev/sd??
  /dev/sdb1  /dev/sdb2

\.. the card is clearly "/dev/sdb".


Mac OS X-Specific Tips
----------------------

The disk should appear as "/dev/diskX" where X is some number (e.g.,
"/dev/disk2").  Individual partitions appear as "/dev/diskXsY" where Y is another
number.

You can use the Disk Utility to find out the disk name
easily.  After opening the utility, find the disk in the frame on the left that
matches the size of your Compact Flash card.  Click on the "Info" icon at the
top of the window, and read the value listed as "Disk Identifier".  Appending
this value to "/dev" will give you the full disk name (e.g., "/dev/disk2").

You can also use the technique of noting appearing paritions as described in
the Linux section (though with names like "/dev/diskXsY").


Windows-Specific Tips
---------------------

Windows does not come with a bzip2 decompressor *or* the dd program, but
Windows versions of both are available:

.. list-table:: Windows Tools
   :header-rows: 1

   * - Tool
     - Homepage

   * - `dd 0.5 <http://www.chrysocome.net/downloads/dd-0.5.zip>`_
     - `chrysocome.net/dd <http://www.chrysocome.net/dd>`_

   * - `Windows bzip2 binaries <http://gnuwin32.sourceforge.net/downlinks/bzip2-bin-zip.php>`_
     - `gnuwin32.sourceforge.net/packages/bzip2.htm <http://gnuwin32.sourceforge.net/packages/bzip2.htm>`_

**Tip**: The easiest way to use these tools is probably to extract the "bin"
folder from the bzip2 binaries zip file, and put dd.exe from the dd zip file
directly into it.  Run cmd.exe from inside this folder.

cmd.exe will need to be run as an Administrator.  One easy way to do this is
simply by logging in as an Administrator.  On Windows Vista, you can run
cmd.exe from the Run box while holding Control and Shift.


To find the appropriate "disk name" in Windows, open the Disk Management
snap-in for Microsoft Management Console (you can do this by running
"diskmgmt.msc" from the Run box).  Find the Compact Flash card in the bottom
frame.  You can identify it by its size and probably by the drive letter
assigned to it.  You can also remove and reinsert the card and note which
entry disappears and reappears.  On the left, this entry will be labeled
"Disk x", where x is some number.  The drive name you will use with dd will be
"\\.\PHYSICALDRIVEx".





Hardware Guide
================

If you wish to build your own NOX Box, this section covers the required hardware components. For your convenience, we also provide links to online retailers.

For the Impatient
-----------------

You can buy everything from Netgate.com for about $285 (not including shipping/tax):

* `PC Engines ALIX 2c3 Board <http://www.netgate.com/product_info.php?cPath=60_83&products_id=450>`_

* `ALIX 2c3 indoor enclosure with 2 side antenna holes <http://www.netgate.com/product_info.php?cPath=67&products_id=596>`_

* `Winston CM9 Atheros-based 802.11a/b/g Mini PCI card <http://www.netgate.com/product_info.php?cPath=26_34&products_id=126>`_

* `Rubber Duck Omni Antenna (buy 2 for dual antenna setup) <http://www.netgate.com/product_info.php?cPath=23_33&products_id=146>`_

* `Matching Antenna Pigtails (buy 2 for dual antenna setup) <http://www.netgate.com/product_info.php?cPath=21_94&products_id=144>`_

* `2 GB Compact Flash Card <http://www.netgate.com/product_info.php?cPath=24_79&products_id=543>`_

* `Power Adapter <http://www.netgate.com/product_info.php?cPath=24_55&products_id=357>`_

Component Details
-----------------

You can save around $40 per NOX Box if you're willing to shop around a bit and buy
components from different retailers. Below we lay out what you need to
know to find own component sources, and we and provide links to the
components we purchased to build our own NOX Boxes.

System Board
^^^^^^^^^^^^^

We use the `PC Engines ALIX 2c3 <http://www.pcengines.ch/alix2c3.htm>`_ board,
which is a low-power single-board computer with a 500 MHz AMD Geode processor,
256 MB of RAM, three onboard Ethernet ports, and a Mini PCI slot. While PC Engines
offers a variety of similar boards with slightly differing hardware, we
suggest that you buy one of the boards with three Ethernet ports, as this
is the configuration we test with the most (and we *really* suggest that you get one with at least two Ethernet ports). PC Engines' other three
port board is the `ALIX 2c1 <http://www.pcengines.ch/alix2c1.htm>`_ which
is similar to the ALIX 2c3, but with half the RAM, no USB, and a somewhat slower processor.

Prices for PC Engines boards seem to be more or less uniform across online
retailers, so we buy ours from a business in the Bay Area: Mini-Box.com (they allow
same day pick up from their Fremont warehouse). Here are the links to the ALIX 2c3
and 2c1 boards, which are priced at $137 and $119 respectively:

* `ALIX 2c3 at Mini-Box.com <http://www.mini-box.com/Alix-2B-Board-3-LAN-3-MINI-PCI_1?sc=8&category=19>`_
* `ALIX 2c1 at Mini-Box.com <http://www.mini-box.com/Alix-2B0-Board-3-LAN-1-MINI-PCI_LX700?sc=8&category=19>`_

System Enclosure
^^^^^^^^^^^^^^^^

Both indoor and outdoor enclosures are available from suppliers like
`Mini-Box.com <http://www.mini-box.com/s.nl/sc.8/category.19/.f>`_ (toward the bottom of the page) or
`Netgate.com <http://www.netgate.com/index.php?cPath=67>`_.
We have only used the indoor enclosures. (If you have experience with outdoor
enclosures, we would be interested in your experience.)

Mini-Box.com offers the indoor enclosure for the ALIX 2c3 board for $12.95, but
contrary to what the product blurb says, there are no antenna holes (the small
hole visible in the picture is for the power supply). We actually buy these
cases and drill two antenna holes in the side ourselves, but if you need
antenna holes, you might want to just pay $21 and get the
`pre-drilled case
<http://www.netgate.com/product_info.php?cPath=67&products_id=596>`_ from
Netgate.com. However, besides being more expensive, this case can also be somewhat inconvenient. Inserting and removing the Compact Flash card requires lifting up the ALIX board, and this can only be done without the antenna pigtails installed. If you are re-flashing the Compact Flash regularly, the constant mounting and unmounting of the pigtails is somewhat burdensome.


Mini PCI Wireless Card
^^^^^^^^^^^^^^^^^^^^^^^

The NOX Box software should work with any modern Atheros card. Non-Atheros
cards *will not work* without some luck and reconfiguration. We have successfully used two different cards: the CM9
from Wistron and the WLM54G from Compex. The Compex supports 802.11 b/g
while the Wistron is a/b/g. Both support WPA and the Wistron claims 802.1x
support (not verified).

The Compex is
`$30 from Mini-Box <http://www.mini-box.com/s.nl/it.A/id.460/.f?sc=8&category=19>`_.
The Wistron is `$40 from Netgate
<http://www.netgate.com/product_info.php?cPath=26_34&products_id=126>`_
but is
`$60 from Mini-Box <http://www.mini-box.com/s.nl/it.A/id.387/.f?sc=8&category=19>`_.

Antennas and Pigtails
^^^^^^^^^^^^^^^^^^^^^

If you use your NOX Box as a wireless access point, you probably want to buy antennas and
pigtails (the component that connects the antenna to your Mini PCI wireless card).
We have not experimented significantly with different setups, but we currently
use dual 3 dBi antennas, which is roughly equivalent to the setup on a standard
home router.

Buying antennas and pigtails separately on sites like Mini-Box and Netgate tends to be
fairly expensive ($8 for the antenna, $14 for the pigtail) so we tend to either
pick up supplies at a local electronics store or buy antennas that include a pigtail already (e.g., this
`omni from streakwave.com <http://www.streakwave.com/Itemdesc.asp?ic=AC%2FSWI&eq=&Tp=>`_
is $15 total).
**Note**: we have not yet tried mounting these with the pre-drilled enclosures from Netgate.

Compact Flash
^^^^^^^^^^^^^^^

Any standard Compact Flash card will suffice for a NOX Box. Our current distribution images are under 1 GB, but we suggest buying a 2 GB card to allow for online image updates, which we expect to support soon. We buy our Compact Flash from a local
electronics store to save a few bucks. If you are going to be developing a lot on your
NOX Box (or your site is particularly prone to power failures), it may be worth purchasing high quality flash with faster read/write speeds
and better endurance in the face of writes and sudden power loss. As of now we have not
had enough experience with Compact Flash failures to give more specific advice.

Power Adapter
^^^^^^^^^^^^^

The ALIX boards work with any generic power adapter in the 7V to 20V range.
We use
`this adapter <http://www.mini-box.com/60w-12v-5A-AC-DC-Power-Adapter_3?sc=8&category=19>`_
($10 from Mini-Box) because we like the form factor, but a power adapter from your local
electronic store will also work. You probably need something along the lines of 500mA at 12V, but you may want to consult PC Engines' documentation to be sure.

The ALIX boards also support passive Power-over-Ethernet (POE), but we have not
experimented with that option.


References
==========
.. [RFC1334] http://www.rfc-archive.org/getrfc.php?rfc=1334
