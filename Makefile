
export ROOT_DIR=$(CURDIR)
export BUILD_DIR=$(ROOT_DIR)/build
export LIB_DIR=$(ROOT_DIR)/common
export RFC_DIR=$(ROOT_DIR)/rf-controller
export INC_DIR=$(ROOT_DIR)/include

export BUILD_LIB_DIR=$(BUILD_DIR)/lib
export BUILD_OBJ_DIR=$(BUILD_DIR)/obj

#the lib subdirs should be done first
export libdirs := rf-protocol sys ipc 
export srcdirs := rf-server rf-slave 

export CPP := g++
export CFLAGS := -O2 -Wall -D_GNU_SOURCE -D_POSIX_SOURCE -W
export AR := ar

all: build lib app nox

build:
	@mkdir -p $(BUILD_DIR);

lib: build
	@mkdir -p $(BUILD_OBJ_DIR);
	@mkdir -p $(BUILD_LIB_DIR);
	@for dir in $(libdirs); do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Library $$dir..."; \
		make -C $(LIB_DIR)/$$dir all; \
		rmdir $(BUILD_OBJ_DIR)/$$dir; \
		echo "done."; \
	done

app: lib
	@mkdir -p $(BUILD_OBJ_DIR);
	@for dir in $(srcdirs); do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Application $$dir..."; \
		make -C $(ROOT_DIR)/$$dir all; \
		echo "done."; \
	done

server: lib
	@mkdir -p $(BUILD_OBJ_DIR);
	@for dir in "rf-server"; do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Application $$dir..."; \
		make -C $(ROOT_DIR)/$$dir all; \
		echo "done."; \
	done
	
slave: lib 
	@mkdir -p $(BUILD_OBJ_DIR);
	@for dir in "rf-slave" ; do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Application $$dir..."; \
		make -C $(ROOT_DIR)/$$dir all; \
		echo "done."; \
	done
	
nox: lib
	echo "Building NOX and RF-Controller..."
	if test -d $(RFC_DIR)/build; \
	then echo "Skipping configure..."; \
	else cd $(RFC_DIR); ./boot.sh; mkdir build; cd build; export CPP=; ../configure;\
	fi
	make -C $(RFC_DIR)/build
	echo "finished."

clean: clean-libs clean-apps_obj clean-apps_bin

clean-nox:
	@rm -rf $(RFC_DIR)/build
	@rm -rf $(RFC_DIR)/autom4te.cache
	@rm -f $(RFC_DIR)/aclocal.m4
	@rm -f $(RFC_DIR)/config.h*
	@rm -f $(RFC_DIR)/configure.ac
	@rm -f $(RFC_DIR)/configure
	@rm -f $(RFC_DIR)/Makefile.in

clean-libs:
	@rm -rf $(BUILD_LIB_DIR)

clean-apps_obj:
	@rm -rf $(BUILD_OBJ_DIR)

clean-apps_bin:
	@rm -rf $(BUILD_DIR)

.PHONY:all lib app nox clean clean-nox clean-libs clean-apps_obj clean-apps_bin
