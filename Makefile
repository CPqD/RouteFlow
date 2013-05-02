export ROOT_DIR=$(CURDIR)
export BUILD_DIR=$(ROOT_DIR)/build
export LIB_DIR=$(ROOT_DIR)/rflib
export NOX_DIR=$(ROOT_DIR)/nox-rfproxy
export MONGO_DIR=/usr/local/include/mongo

export BUILD_LIB_DIR=$(BUILD_DIR)/lib
export BUILD_OBJ_DIR=$(BUILD_DIR)/obj

export RFLIB_NAME=rflib

#the lib subdirs should be done first
export libdirs := ipc types
export srcdirs := rfclient

export CPP := g++
export CFLAGS := -Wall -W
export AR := ar

all: build lib app #nox

build:
	@mkdir -p $(BUILD_DIR);

lib: build
	@mkdir -p $(BUILD_OBJ_DIR);
	@mkdir -p $(BUILD_LIB_DIR);
	@for dir in $(libdirs); do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Library Dependency ($$dir)..."; \
		make -C $(LIB_DIR)/$$dir all || exit 1; \
		echo "done."; \
	done
	@echo "Generating Library";
	make -C $(LIB_DIR) all;

app: lib
	@mkdir -p $(BUILD_OBJ_DIR);
	@for dir in $(srcdirs); do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Application $$dir..."; \
		make -C $(ROOT_DIR)/$$dir all || exit 1; \
		echo "done."; \
	done

rfclient: lib
	@mkdir -p $(BUILD_OBJ_DIR);
	@for dir in "rfclient" ; do \
		mkdir -p $(BUILD_OBJ_DIR)/$$dir; \
		echo "Compiling Application $$dir..."; \
		make -C $(ROOT_DIR)/$$dir all || exit 1; \
		echo "done."; \
	done
	
nox: lib
	echo "Building NOX with rfproxy..."
	cd $(NOX_DIR); \
	export CPP=; \
	make -C $(BUILD_DIR)/nox; \
	echo "done."

clean: clean-libs clean-apps_obj clean-apps_bin #clean-nox

clean-nox:
	@rm -rf $(BUILD_DIR)/nox

clean-libs:
	@rm -rf $(BUILD_LIB_DIR)
	@for dir in $(libdirs); do \
		rm -rf $(BUILD_OBJ_DIR)/$$dir; \
	done

clean-apps_obj:
	@rm -rf $(BUILD_OBJ_DIR)

clean-apps_bin:
	@rm -rf $(BUILD_DIR)

.PHONY:all lib app nox clean clean-nox clean-libs clean-apps_obj clean-apps_bin
