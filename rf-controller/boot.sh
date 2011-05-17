#! /bin/sh -e

have_ext=$(if test -e src/nox/ext/Makefile.am; then echo yes; else echo no; fi)
for opt
do
    case $opt in
        (--apps-core) core_only=yes ;;
        (--apps-net)  net_only=yes ;;
        (--enable-ext) have_ext=yes ;;
        (--disable-ext) have_ext=no ;;
        (--help) cat <<EOF
$0: bootstrap NOX from a Git repository
usage: $0 [OPTIONS]
The recognized options are:
  --enable-ext      include noxext
  --disable-ext     exclude noxext
  --apps-core       only build with core apps
  --apps-net        build with core and net apps (but not webapps)
By default, noxext is included if it is present.
EOF
        exit 0
        ;;
        (*) echo "unknown option $opt; use --help for help"; exit 1 ;;
    esac
done

if test -e src/nox/ext/boot.sh ; then
    (cd src/nox/ext && ./boot.sh)
fi

if test "$core_only" = yes; then
    echo 'building with only core apps'
    cat configure.ac.in | sed -e "s/APPS_ID/core/" | sed -e "s/TURN_ON_NETAPPS/no/" | sed -e "s/TURN_ON_WEBAPPS/no/" > configure.ac
    if test -e ./installer; then
        echo "AC_CONFIG_FILES([installer/Makefile installer/build installer/decompress installer/package/installer " >> configure.ac
    else
        echo "AC_CONFIG_FILES([ " >> configure.ac
    fi
    find . -path "*Makefile.am" | grep -v "\<apache-log4cxx\/" | grep -v \
    "\<ext\>" | grep -v "\<protobuf\/" | grep -v "\<netapps\>" | grep -v "\<webapps\>" \
    | sed -e "s/\.\<am\>//" | sed -e "s/\.\///" \
    >> configure.ac
    echo "])  " >> configure.ac
elif test "$net_only" = yes; then
    echo 'building with core and net apps'
    cat configure.ac.in | sed -e "s/APPS_ID/core/" | sed -e "s/TURN_ON_NETAPPS/yes/" | sed -e "s/TURN_ON_WEBAPPS/no/" > configure.ac
    if test -e ./installer; then
        echo "AC_CONFIG_FILES([installer/Makefile installer/build installer/decompress installer/package/installer " >> configure.ac
    else
        echo "AC_CONFIG_FILES([ " >> configure.ac
    fi

    find . -path "*Makefile.am" | grep -v "\<apache-log4cxx\/" | grep -v \
    "\<ext\>" | grep -v "\<protobuf\/" | grep -v "\<webapps\>" \
    | sed -e "s/\.\<am\>//" | sed -e "s/\.\///" \
    >> configure.ac
    echo "])  " >> configure.ac
else    
    echo 'building with all apps'
    cat configure.ac.in | sed -e "s/APPS_ID/full/" | sed -e "s/TURN_ON_NETAPPS/yes/" | sed -e "s/TURN_ON_WEBAPPS/yes/" > configure.ac
    if test -e ./installer; then
        echo "AC_CONFIG_FILES([installer/Makefile installer/build installer/decompress installer/package/installer " >> configure.ac
    else
        echo "AC_CONFIG_FILES([ " >> configure.ac
    fi
    find . -path "*Makefile.am" | grep -v "\<apache-log4cxx\/" | grep -v \
    "\<ext\>" | grep -v "\<protobuf\/" \
    | sed -e "s/\.\<am\>//" | sed -e "s/\.\///" \
    >> configure.ac
    echo "doc/doxygen/doxygen.conf" >> configure.ac
    echo "])  " >> configure.ac
fi    

# Enable or disable ext.
if test "$have_ext" = yes; then
    echo 'Enabling noxext...'

    echo "" >> configure.ac
    echo "#" >> configure.ac
    echo "# Automatically included by boot.sh" >> configure.ac
    echo "#" >> configure.ac
    echo "if test \"\$noext\" = false; then" >> configure.ac
    echo -n "AC_CONFIG_SUBDIRS([" >> configure.ac
    find src/nox/ext -name "configure.ac" | grep -v "\<protobuf\/"  | sed -e "s/configure.ac//"  \
    | sed -e's%/$%%' >> configure.ac
    echo "])  " >> configure.ac
    echo "fi" >> configure.ac
    echo "" >> configure.ac

else
    echo 'Disabling noxext...'
fi

echo "AC_OUTPUT  " >> configure.ac

# Bootstrap configure system from .ac/.am files
autoreconf -Wno-portability --install -I `pwd`/config --force

