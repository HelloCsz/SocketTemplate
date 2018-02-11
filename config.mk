CC= gcc
CXX= g++
PWD= $(shell pwd)
SRC= ..
BRPC_SRC_DIR= $(SRC)/brpc

brpc-conf:$(BRPC_SRC)/config_brpc.sh
	@echo "Configuring brpc"
	@cd $(BRPC_SRC_DIR) && sh ./config_brpc.sh --headers=/usr/include --libs=/usr/lib

brpc: $(BRPC_SRC_DIR)/Makefile brpc-conf
	@echo "Building brpc"
	@make -C $(BRPC_SRC_DIR)
