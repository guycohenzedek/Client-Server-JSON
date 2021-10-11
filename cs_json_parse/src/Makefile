# src folder

SUBDIRS_HOST := \
common/Host \
thread_pool/Host \
config/Host \
network/Host \
build_parse_data/Host \
server/Host \
client_1/Host \
client_2/Host \
client_3/Host \

SUBDIRS := $(SUBDIRS_HOST)

clean:	$(SUBDIRS)
	rm ../bin/gsi_parse_json_client_1 \
	   ../bin/gsi_parse_json_client_2 \
	   ../bin/gsi_parse_json_client_3 \
	   ../bin/gsi_parse_json_server

dir:
	mkdir -p ../bin
	mkdir -p ../lib
	
all: dir $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all clean $(SUBDIRS)