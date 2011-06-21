# define these targets in your makefile if you wish
# local_all local_depend local_clean depend_install local_install local_distclean local_extraclean:

# and define these variables to impact your build

LOCAL_CFLAGS = -I/usr/include/python2.4 -I/usr/include/python2.4 -fno-strict-aliasing -DNDEBUG -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -D_GNU_SOURCE -fPIC
LOCAL_LDFLAGS= -lpthread -ldl -lutil -lm -lpython2.4
LOCAL_OBJS=freeswitch_python.o mod_python_wrap.o
include ../../../../build/modmake.rules

LINK=$(CXXLINK)

PYMOD=freeswitch
PYTHON_SITE_DIR=/usr/lib/python2.4/site-packages

SWIGCFILE=../../../switch_swig.c
SWIGIFILE=../../../switch_swig.i

reswig: swigclean mod_python_wrap.cpp

swigclean: clean
	rm -f mod_python_wrap.* freeswitch.py

mod_python_wrap.cpp: mod_python_extra.c hack.diff 
	which swig 2>&1 >>/dev/null && swig -python -c++ -I../../../../src/include -oh mod_python_wrap.h -o mod_python_wrap.cpp mod_python.i
	which swig 2>&1 >>/dev/null && echo "#include \"mod_python_extra.c\"" >> mod_python_wrap.cpp
	which swig 2>&1 >>/dev/null && patch -s -p0 -i hack.diff

local_install:
	mkdir -m755 -p $(DESTDIR)$(PYTHON_SITE_DIR)
	cp -f freeswitch.py $(DESTDIR)$(PYTHON_SITE_DIR)

