# cs_json_parse folder

SUBDIRS := src

all: dir $(SUBDIRS)

clean:	$(SUBDIRS)

dir:
	mkdir -p bin
	mkdir -p lib
	
all_j: json
	@$(MAKE) all
	
clean_j:
	@$(MAKE) -C src/json-c/json-c-build	clean
	@$(MAKE) clean
	
json:
	@$(MAKE) -C src/json-c/json-c-build

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all clean $(SUBDIRS) all_j clean_j

help:
	@echo ""
	@echo "CS-JSON-PARSE Makefile"
	@echo "======================"
	@echo "make all      -- Compile all libs, bins. without JSON-C"
	@echo "make all_j 	 -- Compile all libs, bins and JSON-C library"
	@echo "make clean    -- Clean all object files, bins and libs"
	@echo "make clean_j  -- Clean all object files, bins, libs and JSON_C library"
	@echo ""