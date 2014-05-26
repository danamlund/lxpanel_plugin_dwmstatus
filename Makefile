VERSION=1.2
NAME=dwmstatus-lxpanel-plugin
INPUT=dwmstatus.c
OUTPUT=dwmstatus.so

all: ${OUTPUT} dwmstatusda

${OUTPUT}: ${INPUT}
	./check_required.sh
	gcc -O2 -Wall `pkg-config --cflags glib-2.0 gtk+-2.0` \
	    -shared -fPIC ${INPUT} -o ${OUTPUT} \
	    `pkg-config --libs glib-2.0 gtk+-2.0`

clean:
	rm -f ${OUTPUT} dwmstatusda

dwmstatusda: dwmstatusda.c
	gcc -Os -Wall -pedantic -std=c99 dwmstatusda.c -lX11 -o dwmstatusda

install: ${OUTPUT}
	./install.sh

package:
	rm -Rf ${NAME}-${VERSION}
	mkdir ${NAME}-${VERSION}
	cp -Rf README Makefile ${INPUT} COPYING check_required.sh \
               install.sh lxpanel \
               ${NAME}-${VERSION}
	tar czvf ${NAME}-${VERSION}.tar.gz \
		 ${NAME}-${VERSION}
	rm -Rf ${NAME}-${VERSION}
