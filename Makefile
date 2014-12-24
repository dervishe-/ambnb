# ambnb
#
KMOD	= ambnb
SRCS	= ambnb.c 
SRCS   += device_if.h bus_if.h

.include <bsd.kmod.mk>
