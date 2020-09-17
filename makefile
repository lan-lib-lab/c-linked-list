src = $(wildcard src/*.c)
obj = $(patsubst src/%, build/obj/%, $(src:.c=.o))
release_obj = $(patsubst src/%, build/release/obj/%, $(src:.c=.o))
test_obj = $(patsubst src/%, build/test/obj/%, $(src:.c=.o))
dep = $(obj:.o=.d)
release_dep = $(release_obj:.o=.d)
test_dep = $(test_obj:.o=.d)
libs = $(patsubst lib%, -l%, $(notdir $(basename $(wildcard libs/*.a) $(wildcard libs/*/*.a)))) \
	-lcunit
exec = $(notdir $(CURDIR))
version = 0.1.0
repo_base = $(shell git rev-parse --show-toplevel)

CFLAGS_INCLUDE_LIB_HEADERS = -Iinc $(patsubst %, -I%, $(dir $(wildcard libs/*/)))
LDFLAGS_INCLUDE_LIB_DIRS = -Llibs $(patsubst %, -L%, $(dir $(wildcard libs/*/)))
CFLAGS = -Wall -ggdb $(CFLAGS_INCLUDE_LIB_HEADERS)
LDFLAGS = $(LDFLAGS_INCLUDE_LIB_DIRS) $(libs)

# for testing, enable conditional compilation macro to compile CUnit tests
ifeq ($(MAKECMDGOALS), test)
	CFLAGS += -D CUNIT_TESTS
endif

# disable or enable DEBUG based on release target
ifneq ($(MAKECMDGOALS), release)
	CFLAGS += -D DEBUG
endif


all: build

.PHONY: build
build: clean_build $(obj)
	@mkdir -p build/
	$(CC) $(CFLAGS) -o build/${exec}-$(version) $(obj) $(LDFLAGS)

.PHONY: release
release: clean_release $(release_obj)
	@mkdir -p build/release/
	$(CC) $(CFLAGS) -o build/release/$(exec)-$(version) $(release_obj) $(LDFLAGS)

.PHONY: test
test: clean_test $(test_obj)
	@mkdir -p build/test/
	$(CC) $(CFLAGS) -o build/test/$(exec)-$(version) $(test_obj) $(LDFLAGS)
	build/test/$(exec)-$(version) $(args)

.PHONY: clean_test
clean_test:
	rm -f $(test_obj)
	rm -f $(test_dep)

.PHONY: clean_release
clean_release:
	rm -f $(release_obj)
	rm -f $(release_dep)

.PHONY: clean_build
clean_build:
	rm -f $(obj)
	rm -f $(dep)

.PHONY: clean
clean: clean_build clean_release clean_test

.PHONY: run
run: build
	build/$(exec)-$(version) $(args)

# rule for creating a library in case this is a library project
# an assumption is made that intead of main, a library requires a "lib.c"
# this is used so that this rule isn't accidentally ran on projects that aren't meant to be library projects
lib: $(release_obj)
ifeq (,$(wildcard src/lib.c))
	$(error "making a static library in a project that doesn't contain a lib.c!")
endif
	mkdir -p build/
	rm -rf build/lib$(exec)-$(version)/
	mkdir -p build/lib$(exec)-$(version)/
	mkdir -p build/lib$(exec)-$(version)/$(exec)/
	$(AR) rcs build/lib$(exec)-$(version)/lib$(exec)-$(version).a $^
	cp inc/* build/lib$(exec)-$(version)/$(exec)/

# include dep files in the makefile
ifneq (,$(wildcard build/obj/*))
-include $(dep)
endif
ifneq (,$(wildcard build/release/obj/*))
-include $(release_dep)
endif
ifneq (,$(wildcard build/test/obj/*))
-include $(test_dep)
endif

build/obj/%.o: src/%.c
	@mkdir -p build/obj/
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/release/obj/%.o: src/%.c
	@mkdir -p build/release/obj/
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/test/obj/%.o: src/%.c
	@mkdir -p build/test/obj/
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
build/obj/%.d: src/%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

build/release/obj/%.d: src/%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

build/test/obj/%.d: src/%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@


# this macro builds a library module and installs it in libs/
# it also takes care of removing old versions of the library from libs/
# (lib_path, lib_version)
define install_lib
	$(MAKE) -C $(1)/ lib
	rm -rf libs/lib$(notdir $(1))-*
	cp -r $(1)/build/lib$(notdir $(1))-$(2) libs/
endef

# rule for installing prerequesites like static libraries before execution
.PHONE: install
install:
	@mkdir -p libs
	$(call install_lib,$(repo_base)/c/cunit_utils,0.2.0)

