# Tools
CC				=	gcc
LD				=	gcc
RM				=	rm -f

# BUILD MODE
BUILDMODE		=	debug

# Build flags
ifeq ($(BUILDMODE), debug)
	CFLAGS=-DDEBUG -g -Wall
else
	CFLAGS=-O2 -falign-functions -fomit-frame-pointer
endif
LDFLAGS			=	-lXxf86vm -lXxf86dga -lGLU -lGL
LIBS			=	-L/usr/X11R6/lib

OUT_EXE			=	./demo

VERSION			=	1.0

all:	build

############################################################
# Build                                                    
############################################################
OBJS	=	demo.o \
            common_gl.o \
            common_linux.o \
            common.o \
            extgl.o

DO_CC	=	$(CC) $(CFLAGS) -o $@ -c $<

build:	$(OBJS)
	$(LD) $(LIBS) $(OBJS) $(LDFLAGS) -o $(OUT_EXE)

demo.o:			demo.c
	$(DO_CC)

common_gl.o:	common_gl.c
	$(DO_CC)

common_linux.o:	common_linux.c
	$(DO_CC)

common.o:		common.c
	$(DO_CC)

extgl.o:		extgl.c
	$(DO_CC)

############################################################
# misc
############################################################
package:
	(cd .. && cp -R demo demotemplate-$(VERSION))
	(cd ../demotemplate-$(VERSION) && make allclean)
	(rm -f ../demotemplate-$(VERSION)/data/autoexec.cfg && rm -f ../demotemplate-$(VERSION)/demorun.log)
	(cd .. && tar -cvvf demotemplate-$(VERSION)-src.tar demotemplate-$(VERSION))
	(cd .. && bzip2 -c demotemplate-$(VERSION)-src.tar > demotemplate-$(VERSION)-src.tar.bz2)
	(cd .. && rm -f demotemplate-$(VERSION)-src.tar)
	(cd .. && zip -r -6 demotemplate-$(VERSION)-src.zip demotemplate-$(VERSION))
	(cd .. && rm -rf demotemplate-$(VERSION) && cd demo && mv ../demotemplate-$(VERSION)-src.* .)

binpackage:
	(cd .. && cp -R demo demotemplate-$(VERSION))
	(cd ../demotemplate-$(VERSION) && make allclean)
	(rm -f ../demotemplate-$(VERSION)/data/autoexec.cfg && rm -f ../demotemplate-$(VERSION)/demorun.log)
	(cd ../demotemplate-$(VERSION) && make)
	(cd .. && tar -cvvf demotemplate-$(VERSION)-linux-x86-$(BUILDMODE).tar demotemplate-$(VERSION)/data demotemplate-$(VERSION)/demo demotemplate-$(VERSION)/COPYING.txt demotemplate-$(VERSION)/README.txt)
	(cd .. && bzip2 -c demotemplate-$(VERSION)-linux-x86-$(BUILDMODE).tar > demotemplate-$(VERSION)-linux-x86-$(BUILDMODE).tar.bz2)
	(cd .. && rm -f demotemplate-$(VERSION)-linux-x86-$(BUILDMODE).tar)
	(cd .. && rm -rf demotemplate-$(VERSION) && cd demo && mv ../demotemplate-$(VERSION)-linux-x86-$(BUILDMODE).tar.bz2 .)

clean:
	$(RM) $(OBJS)

allclean:	clean
	$(RM) $(OUT_EXE)
	$(RM) *~ */*~ ../*~ ../*/*~
	$(RM) -r demotemplate-$(VERSION)-src.*
	$(RM) -r demotemplate-$(VERSION)-linux-x86-*.tar.bz2