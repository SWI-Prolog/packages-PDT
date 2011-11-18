################################################################
# Build the SWI-Prolog PDT package for MS-Windows
#
# Author: Jan Wielemaker
#
# Use:
#	nmake /f Makefile.mak
#	nmake /f Makefile.mak install
################################################################

PLHOME=..\..
!include $(PLHOME)\src\rules.mk
CFLAGS=$(CFLAGS) /D__SWI_PROLOG__

PDTOBJ=		pdt_console.obj

all:		pdt_console.dll

pdt_console.dll:	$(PDTOBJ)
		$(LD) /dll /out:$@ $(LDFLAGS) $(PDTOBJ) $(PLLIB) $(LIBS)

install:	idll ilib

################################################################
# Testing
################################################################

check::

################################################################
# Installation
################################################################

idll::
		copy pdt_console.dll "$(BINDIR)"

ilib::
		copy pdt_console.pl "$(PLBASE)\library"
		$(MAKEINDEX)

uninstall::
		del "$(BINDIR)\pdt_console.dll"
		del "$(PLBASE)\library\pdt_console.pl"
		$(MAKEINDEX)

html-install::
		-copy pdt.html "$(PKGDOC)"

xpce-install::

clean::
		if exist *.obj del *.obj
		if exist *~ del *~

distclean:	clean
		-DEL *.dll *.lib *.exp *.ilk *.pdb 2>nul


