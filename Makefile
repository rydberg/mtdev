VERSION = 1
PATCHLEVEL = 0
EXTRAVERSION = beta1

LIBRARY	= libmtdev.so
MODULES = src

o_src	= match iobuf caps core

TARGETS	+= test/mtdev-mapgen
TARGETS	+= test/mtdev

OBJECTS	= $(addsuffix .o,\
	$(foreach mod,$(MODULES),\
	$(addprefix $(mod)/,$(o_$(mod)))))

TBIN	= $(addprefix bin/,$(TARGETS))
TLIB	= $(addprefix obj/,$(LIBRARY))
TOBJ	= $(addprefix obj/,$(addsuffix .o,$(TARGETS)))
OBJS	= $(addprefix obj/,$(OBJECTS))
LIBS	= 

DLIB	= usr/lib

INCLUDE = -Iinclude
OPTS	= -O3 -fPIC

.PHONY: all clean
.PRECIOUS: obj/%.o

all:	$(OBJS) $(TLIB) $(TOBJ) $(TBIN)

bin/%:	obj/%.o $(TLIB)
	@mkdir -p $(@D)
	gcc $< -o $@ $(TLIB) $(LIBS)

$(TLIB): $(OBJS) $(XOBJS)
	@rm -f $(TLIB)
	gcc -shared $(OBJS) $(XOBJS) -Wl,-soname -Wl,$(LIBRARY) -o $@

obj/%.o: %.c
	@mkdir -p $(@D)
	gcc $(INCLUDE) $(OPTS) -c $< -o $@

clean:
	rm -rf bin obj

distclean: clean
	rm -rf debian/*.log debian/files

install: $(TLIB) $(TFDI)
	install -d "$(DESTDIR)/$(DLIB)"
	install -m 755 $(TLIB) "$(DESTDIR)/$(DLIB)"
