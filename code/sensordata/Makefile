ifndef TARGET
TARGET=sensinode
endif

# Make absolutely certain that you specify your device here
DEFINES=MODEL_N740

# This example doesn't need code banking so we turn it off 
#HAVE_BANKING=1

CONTIKI_PROJECT = sensors-example
PROJECT_SOURCEFILES += sensor_value.c

all: $(CONTIKI_PROJECT) 

CONTIKI = ../..

include $(CONTIKI)/Makefile.include
