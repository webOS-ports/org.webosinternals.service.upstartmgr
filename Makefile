VERSION=unknown

STAGING_DIR=/srv/preware/build/staging/i686
CC=/srv/optware/i686g25/toolchain/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc

CPPFLAGS := -DVERSION=\"${VERSION}\" -I${STAGING_DIR}/usr/include/glib-2.0 -I${STAGING_DIR}/usr/lib/glib-2.0/include -I${STAGING_DIR}/usr/include
LDFLAGS  := -L${STAGING_DIR}/usr/lib -llunaservice -lmjson -lglib-2.0

upstartmgr: upstartmgr.o luna_service.o luna_methods.o

install: upstartmgr
	scp upstartmgr root@webos:/var/usr/bin/upstartmgr

clobber:
	rm -rf *.o upstartmgr
