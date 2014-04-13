#########
#
# Top level Makefile.
# Mostly just recurse into subdirectories.

SUBDIR-y += src/ledscape
SUBDIR-y += src/demos
SUBDIR-y += src/mta
SUBDIR-y += src/net

all: 
	for dir in $(SUBDIR-y); do \
		$(MAKE) -C $$dir; \
	done

clean:
	for dir in $(SUBDIR-y); do \
		$(MAKE) -C $$dir clean; \
	done

