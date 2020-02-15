ifndef TARGET
TARGET=sensinode
endif

# Make absolutely certain that you specify your device here
DEFINES=MODEL_N740

# These examples don't need code banking so we turn it off 
#HAVE_BANKING=1

CONTIKI_PROJECT = dest source
PROJECT_SOURCEFILES += multihop-callbacks.c util.c sensor-value.c

all: $(CONTIKI_PROJECT) 

CONTIKI = /home/Admin/contiki-2.x-sensinode-20101012/
include $(CONTIKI)/Makefile.include
