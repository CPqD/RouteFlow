#! /bin/sh -e
#if test ! -d $2; then
#    mkdir -p $2
#fi 

swig -outdir $2 -o $1_wrap.cc.tmp1 -c++ -python -module $1 $3

# This makes the file read-only, by default, in Emacs, so that you
# don't end up editing it by mistake.  (You can always use C-x C-q to
# turn off read-only mode if you really want to.)
cat - $1_wrap.cc.tmp1 > $1_wrap.cc.tmp2 <<'EOF'
/* -*- mode: c++; buffer-read-only: t -*- */
EOF

mv $1_wrap.cc.tmp2 $1_wrap.cc
rm -f $1_wrap.cc.tmp1
