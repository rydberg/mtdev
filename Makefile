VERSION = 1
PATCHLEVEL = 0
EXTRAVERSION = beta1

LIBRARY	= libmtdev
MODULES = src

o_src	= match iobuf caps core

TARGETS	+= test/mtdev-mapgen
TARGETS	+= test/mtdev

OBJECTS	= $(addsuffix .o,\
	$(foreach mod,$(MODULES),\
	$(addprefix $(mod)/,$(o_$(mod)))))

TBIN	= $(addprefix bin/,$(TARGETS))
SLIB	= $(addprefix obj/,$(LIBRARY).a)
DLIB	= $(addprefix obj/,$(LIBRARY).so)
TOBJ	= $(addprefix obj/,$(addsuffix .o,$(TARGETS)))
OBJS	= $(addprefix obj/,$(OBJECTS))
LIBS	= 

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

install: $(SLIB) $(DLIB)
	install -d $(DESTDIR)/$(DESTLIB)
	install -m 755 $(SLIB) $(DESTDIR)/$(DESTLIB)
	install -m 755 $(DLIB) $(DESTDIR)/$(DESTLIB)
	ldconfig -n $(DESTDIR)/$(DESTLIB)
