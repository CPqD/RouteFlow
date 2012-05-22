#  http://autoconf-archive.cryp.to/ac_pkg_swig.html 
#
#  License:
#
#  Copyright © 2008 Sebastian Huber <sebastian-huber@web.de>
#  Copyright © 2008 Alan W. Irwin <irwin@beluga.phys.uvic.ca>
#  Copyright © 2008 Rafael Laboissiere <rafael@laboissiere.net>
#  Copyright © 2008 Andrew Collier <colliera@ukzn.ac.za>
#  
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or (at
#  your option) any later version.
#  
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
#  
#  As a special exception, the respective Autoconf Macro's copyright
#  owner gives unlimited permission to copy, distribute and modify the
#  configure scripts that are the output of Autoconf when processing the
#  Macro. You need not follow the terms of the GNU General Public
#  License when using or distributing such scripts, even though portions
#  of the text of the Macro appear in them. The GNU General Public
#  License (GPL) does govern all other use of the material that
#  constitutes the Autoconf Macro.
#  
#  This special exception to the GPL applies to versions of the Autoconf
#  Macro released by the Autoconf Macro Archive. When you make and
#  distribute a modified version of the Autoconf Macro, you may extend
#  this special exception to the GPL to apply to your modified version
#  as well.
#  

AC_DEFUN([AC_PROG_SWIG],[
        AC_PATH_PROG([SWIG],[swig])
        if test -z "$SWIG" ; then
                AC_MSG_WARN([cannot find 'swig' program. You should look at http://www.swig.org])
                SWIG=''
        elif test -n "$1" ; then
                AC_MSG_CHECKING([for SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        if test $available_major -ne $required_major \
                                -o $available_minor -ne $required_minor \
                                -o $available_patch -lt $required_patch ; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.  You should look at http://www.swig.org])
                                SWIG=''
                        else
                                AC_MSG_NOTICE([SWIG executable is '$SWIG'])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_NOTICE([SWIG library directory is '$SWIG_LIB'])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG=''
                fi
        fi
        AC_SUBST([SWIG_LIB])
])



AC_DEFUN([SWIG_ENABLE_CXX],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PROG_CXX])
        SWIG="$SWIG -c++"
])



AC_DEFUN([SWIG_PYTHON],[
        AC_MSG_CHECKING([for SWIG 64-bit support])
        AC_EGREP_CPP(yes, [
#include <stdint.h>
#if __WORDSIZE == 64
yes
#endif
], swig_64bit=yes, swig_64bit=no)
        AC_MSG_RESULT([$swig_64bit])

        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        if test "$swig_64bit" == "yes"; then
           enable_swig_64bit=" -DSWIGWORDSIZE64"
        fi

        AC_SUBST([SWIG_PYTHON_OPT],["-python$swig_shadow $enable_swig_64bit"])
        AC_SUBST([SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])

])

