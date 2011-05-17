.. _installation:

NOX Installation
===============================

Quick Start From Git
--------------------

For those with all the proper dependencies installed (see below) NOX can
be configured and built using standard autoconf procedure:: 

   git clone git://noxrepo.org/noxcore
   cd noxcore/
   ./boot.sh
   mkdir build/
   cd build/
   ../configure 
   make
   make check

If building from the source tarfile, you don't need to run boot.sh.

By default, NOX builds with C++ STL debugging checks, which can slow
down execution speeds by a factor of 10.  For a production build, 
you will want to turn this off::

   ./configure --enable-ndebug

Once compiled, the *nox_core* binary will be built in the *src/*
directory.  **Note** that nox_core **must** be run from the *src/*
build directory, and that the build directory must not be moved to a
different place in the file system.  You can verify that it has built
properly by printing out the usage information::

    cd src/
    ./nox_core -h

If you've gotten this far, then you're ready to test your build
(:ref:`install_test`) or get right to using it (:ref:`sec_use`).

Not So Quick Start
-------------------

Dependencies
^^^^^^^^^^^^^^

The NOX team's internal development environment is standardized around
Debian's Lenny distribution (http://wiki.debian.org/DebianLenny).  While
we test releases on other Linux distributions (Fedora, Gentoo, Ubuntu),
FreeBSD and NetBSD, using Lenny is certain to provide the least hassle. 

NOX relies on the following software packages.  All are available under
Debian as apt packages. Other distributions may require them to be
separately installed from source:

* g++ 4.2 or greater
* Boost C++ libraries, v1.34.1 or greater (http://www.boost.org)
* Xerces C++ parser, v2.7.0 or greater (http://xerces.apache.org/xerces-c)

For Twisted Python support (highly recommended) the following additional packages are required.

* SWIG v1.3.0 or greater (http://www.swig.org)
* Python2.5 or greater (http://python.org)
* Twisted Python (http://twistedmatrix.com)

The user interface (web management console) requires

* Mako Templates (http://www.makotemplates.org/)
* Simple JSON (http://www.undefined.org/python/)

.. warning::
   Older versions of swig may have incompatibilities with newer gcc/g++
   versions.  This is known to be a problem with g++4.1 and swig v1.3.29

Boot Options
^^^^^^^^^^^^^

If building directly from git, the build system needs to be
bootstrapped.  

  ./boot 


Configure Options
^^^^^^^^^^^^^^^^^^

The following options are commonly used in NOX configuration.  Use
./configure ----help for a full listing: 

``--with-python=[yes|no|/path/to/python]`` By default, NOX builds with
an embedded Python interpreter.  You can use this option to specify
which Python installation to use, or to build without Python support
(by using --with-python=no).  NOX requires that the embedded python
support twisted.

``--enable-ndebug`` This will turn *off* debugging (STL debugging in
particular) and increase performance significantly.  Use this whenever
running NOX operationally.

Distribution Specific Installation Notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Debian 5.0 (Lenny):**

Compiling requires the following packages:: 

  apt-get install autoconf automake g++ libtool python python-twisted
  swig libboost1.35-dev libxerces-c2-dev libssl-dev make
  
Running the web server requires json support in python::
  
  apt-get install python-simplejson

To build the documentation you will need to install sphinx::

  apt-get install python-sphinx

**Ubuntu 8.04:**

Compiling requires the following packages:: 

    apt-get install autoconf automake g++ libtool python python2.5-dev python-twisted
    swig libboost-python1.34.1 libboost-python-dev libboost-serialization-dev
    libboost-test-dev libxerces28-dev libssl-dev make

Running the web server requires json support in python::

    apt-get install python-simplejson

**Ubuntu 9.04:**

Compiling requires the following packages:: 

    apt-get git-core install autoconf automake g++ libtool python python-dev 
    python-twisted swig libssl-dev make libboost-dev libxerces-c2-dev

Running the web server requires json support in python::

    apt-get install python-simplejson

**Fedora Core 9:+**

From a standard development install, you can build
after installing the following packages::
  
  yum install xerces-c-devel python-twisted libpcap-devel

**Gentoo 2008.0-rc1**

To compile without twisted python you'll need the following packages::
    
  - emerge -av boost
  - emerge -av xerces-c

**OpenSUSE 10.3 :**

The boost distribution that comes with OpenSuse is too old.  You'll have
to install this from the source:

* boost (http://www.boost.org)

To build NOX (with twisted python) you'll have to installed the
following packages from a base install::

  gcc gcc-c++ make libXerces-c-27 libXerces-c-devel
  libpcap-devel libopenssl-devel swig python-devel 
  python-twisted python-curses 

**Mandriva One 2008:**

NOX compiled on Mandriva with the following packages installed::

  libboost-devel boost-1.35.0 libxerces-c-devel
  libopenssl0.9.8-devel libpython2.5-devel
  python-twisted swig-devel

If the swig and swig-devel packages are not available from the repository, you
will have to build swig from source.

.. _install_test:

Testing your build
^^^^^^^^^^^^^^^^^^^^

You can verify that NOX built correct by running::

    make check

From the build directory.  Unittests can be run independently through
the *test* application::

    cd src
    ./nox_core tests

As a simple example, if you've compiled with Twisted try running
*packetdump* using generated traffic::

    cd src/
    ./nox_core -v -i pgen:10 packetdump

This should print out a description of ten identical packets,
and then wait for you to terminate *nox_core* with 'Ctrl-c'.
