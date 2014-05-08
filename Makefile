VERSION=1.0

all: dwmstatus.so

dwmstatus.so: dwmstatus.c
	gcc -O2 -Wall `pkg-config --cflags lxpanel gtk+-2.0` \
	-shared -fPIC dwmstatus.c -o dwmstatus.so \
	`pkg-config --libs lxpanel gtk+-2.0`

clean:
	rm -f dwmstatus.so

install: 
	@ if [ "$(DESTDIR)" ]; then \
	  cp -v dwmstatus.so $(DESTDIR); \
	elif [ -d "/usr/lib/lxpanel/plugins" ]; then \
	  cp -v dwmstatus.so /usr/lib/lxpanel/plugins; \
	elif [ -d "/usr/lib64/lxpanel/plugins" ]; then \
	  cp -v dwmstatus.so /usr/lib64/lxpanel/plugins; \
	elif [ -d "/usr/lib/i386-linux-gnu/lxpanel/plugins" ]; then \
	  cp -v $(FILE) /usr/lib/i386-linux-gnu/lxpanel/plugins; \
	elif [ -d "/usr/lib64/i386-linux-gnu/lxpanel/plugins" ]; then \
	  cp -v $(FILE) /usr/lib64/i386-linux-gnu/lxpanel/plugins; \
	else \
	  echo ;\
	  echo Couldn\'t find lxpanel/plugins directory.; \
	  echo Checked /usr/lib/lxpanel/plugins and /usr/lib64/lxpanel/plugins; \
	  echo and /usr/lib/i386-linux-gnu/lxpanel/plugins; \
	  echo and /usr/lib64/i386-linux-gnu/lxpanel/plugins.; \
	  echo Find it yourself by running \'locate deskno.so\'; \
	  echo Then copy dwmstatus.so to that directory.; \
	fi

