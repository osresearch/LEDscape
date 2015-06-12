SCRIPTDIR = ../src/script/
BINDIR = ../../bin/

scripts = bbb-network-setup \
	find-serial \
	install \
	ledscape.service \
	python-test \
	run-ledscape \
	run-videoplayer \
	twitter-scroller \
	videoplayer \
	videoplayer.service

all: $(scripts)

.PHONY: $(scripts)

$(scripts): 
	if test ! -e $(BINDIR)$@; \
	then ln -s $(SCRIPTDIR)$@ $(BINDIR); \
	fi
