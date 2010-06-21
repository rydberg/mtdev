VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION =

LIBRARY	= libmtdev
MODULES = src

o_src	= match iobuf caps core

TARGETS	+= test/mtdev-mapgen
TARGETS	+= test/mtdev
PUBINC	+= mtdev.h

OBJECTS	= $(addsuffix .o,\
	$(foreach mod,$(MODULES),\
	$(addprefix $(mod)/,$(o_$(mod)))))

TBIN	= $(addprefix bin/,$(TARGETS))
SLIB	= $(addprefix obj/,$(LIBRARY).a)
DLIB	= $(addprefix obj/,$(LIBRARY).so)
TOBJ	= $(addprefix obj/,$(addsuffix .o,$(TARGETS)))
TINC	= $(addprefix include/,$(PUBINC))
OBJS	= $(addprefix obj/,$(OBJECTS))
LIBS	= 

DESTINC	= usr/include
DESTLIB	= usr/lib

INCLUDE = -Iinclude
OPTS	= -O3 -fPIC

.PHONY: all clean
.PRECIOUS: obj/%.o

all:	$(OBJS) $(SLIB) $(DLIB) $(TOBJ) $(TBIN)

bin/%:	obj/%.o $(SLIB)
	@mkdir -p $(@D)
	gcc $< -o $@ $(SLIB) $(LIBS)

$(SLIB): $(OBJS) $(XOBJS)
	@rm -f $(SLIB)
	ar qc $@ $(OBJS) $(XOBJS)

$(DLIB): $(OBJS) $(XOBJS)
	@rm -f $(DLIB)
	gcc -shared $(OBJS) $(XOBJS) -Wl,-soname -Wl,$(LIBRARY).so -o $@

obj/%.o: %.c
	@mkdir -p $(@D)
	gcc $(INCLUDE) $(OPTS) -c $< -o $@

clean:
	rm -rf bin obj

distclean: clean
	rm -rf debian/*.log debian/files

install: $(TINC) $(SLIB) $(DLIB)
	install -d $(DESTDIR)/$(DESTINC)
	install -m 644 $(TINC) $(DESTDIR)/$(DESTINC)
	install -d $(DESTDIR)/$(DESTLIB)
	install -m 644 $(SLIB) $(DESTDIR)/$(DESTLIB)
	install -m 755 $(DLIB) $(DESTDIR)/$(DESTLIB)
	ldconfig -n $(DESTDIR)/$(DESTLIB)

uninstall:
	rm -f $(DESTDIR)/$(DESTLIB)/$(LIBRARY).so
	rm -f $(DESTDIR)/$(DESTLIB)/$(LIBRARY).a
	rm -f $(DESTDIR)/$(DESTINC)/$(PUBINC)
