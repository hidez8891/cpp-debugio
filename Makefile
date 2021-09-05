all: build

ifdef MAKEDIR:
!ifdef MAKEDIR

include Makefile-nmake.mk

!else
else

include Makefile-gmake.mk

endif
!endif :
