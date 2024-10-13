#
# Create a file called user.prefs by copying the example-user.prefs file
# to user.prefs and editing its values.
#
DIR_BUILD :=  $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
DIR_SRC := $(realpath $(DIR_BUILD)/../../src)
DIR_INCLUDE := $(realpath $(DIR_BUILD)/../../include)

ifeq ($(DIR_BUILD_CONFIG),)
export DIR_BUILD_CONFIG := $(realpath $(DIR_BUILD)/../../../../build-config/linux)
endif


include user.prefs

MODNAME      ?= simple_template
SHARED_FNAME := mod_$(MODNAME).so
CC	:= gcc
OUTPUT_DIR	 := out
SHARED       := $(OUTPUT_DIR)/$(SHARED_FNAME)

APXS := $(DIR_APACHE)/bin/apxs

APACHE_INCLUDES_DIR := $(APACHE_DIR)/include

-include $(DIR_BUILD_CONFIG)/project.properties

ifeq ($(strip $(APXS)),)
APXS	:= $(shell which apxs)
endif


# XXX: These are currently unused as we rely on the apxs utility for module
#      installation.
INSTALL_DIR  ?= $(APACHE_DIR)/modules
INSTALLED    := $(INSTALL_DIR)/mod_$(MODNAME).so
BUILD_DIR := build

CFILES := \
	mod_simple_template.c

VPATH := $(DIR_SRC)
HFILES := $(CFILES:%.c=%.h)
SRCFILES := $(CFILES) $(HFILES)
OBJFILES := $(CFILES:%.c=$(OUTPUT_DIR)/%.o)

INCLUDE_PATHS += \
	$(APACHE_DIR)/include \
	$(DIR_INCLUDE) \
	$(DIR_JANSSON)/include \
	$(DIR_GRASSROOTS_UTIL)/include \
	$(DIR_GRASSROOTS_UTIL)/include/containers \
	$(DIR_GRASSROOTS_UTIL)/include/io \


LIB_PATHS += $\
	$(DIR_JANSSON)/lib \
	$(DIR_GRASSROOTS_UTIL_LIB) \

LIBS += \
	jansson \
	grassroots_util
	
MACROS := \
	UNIX \
	
WARNINGS :=                           \
	all                           \
	extra                         \
	no-unused-parameter           \
	no-missing-field-initializers \
	no-format                     \
	fatal-errors \
	shadow

CFLAGS +=                              \
	-g3                            \
	-ggdb                          \
	-O0							\
	-pedantic                      \
	$(addprefix -W, $(WARNINGS))   \
	$(addprefix -D, $(MACROS)) \
	$(addprefix -I, $(INCLUDE_PATHS))

LDFLAGS +=                           \
	$(addprefix -l, $(LIBS))     \
	$(addprefix -L, $(LIB_PATHS)) \
	-shared 

comma := ,

.PHONY: all shared install test clean init

all: init shared




init:
	mkdir -p $(OUTPUT_DIR)

install: $(SHARED)
	cp $(SHARED) $(INSTALL_DIR)/

shared: $(SHARED)

$(SHARED): init $(OBJFILES)
	@echo "LIB_PATHS: $(LIB_PATHS)"
	@echo "OBJFILES: $(OBJFILES)"
	$(LD) -o $(SHARED) $(OBJFILES) $(STATIC_LIBS) $(LDFLAGS)

clean:
	rm -rvf $(OUTPUT_DIR)/*

	
#$(SHARED): apxs $(SRCFILES) 
#	$(APXS) -c   \
#	$(addprefix -Wc$(comma), $(CFLAGS))  \
#	$(addprefix -Wl$(comma), $(LDFLAGS)) \
#	-o $(SHARED_FNAME) $(SRCFILES)  

$(OUTPUT_DIR)/%.o: %.c 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -fPIC -DLINUX -D_REENTRANT -D_GNU_SOURCE $< -o $@


