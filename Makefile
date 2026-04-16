include config.mk

SRCS = emutui.c drw.c filehandler.c log.c
OBJS = $(SRCS:.c=.o)

emutui: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c config.h config.mk
	$(CC) $(CFLAGS) -c $< -o $@

config.h:
	cp config.def.h $@

config.mk:
	cp config.def.mk $@

clean:
	rm -f $(OBJS) emutui

install: emutui
	mkdir -p $(MAKE_HOME)/.config/emutui/menu
	cp ./config/menu/main.entry $(MAKE_HOME)/.config/emutui/menu/
	cp ./config/scan.types $(MAKE_HOME)/.config/emutui/
	chown -R $(MAKE_USER):$(MAKE_GROUP) $(MAKE_HOME)/.config/emutui
	cp -f emutui $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/emutui

install-postclean: install clean

uninstall:
	rm -f $(PREFIX)/bin/emutui

purge: uninstall
	rm -rf $(MAKE_HOME)/.config/emutui/

rebuild: clean emutui

reset: purge clean
	rm -f config.h
	rm -f config.mk

.PHONY: clean install install-postclean uninstall purge rebuild reset