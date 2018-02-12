CC= gcc
CXX= g++

PWD= $(shell pwd)
SRC= ..

BRPC_SRC_DIR= $(SRC)/brpc

PROTOC= $(shell which protoc)
PROTOS_DIR= $(SRC)/proto
PROTOS= $(wildcard $(PROTOS_DIR)/*.proto)
PROTO_GENS= $(PROTOS:.proto=.pb.h) $(PROTOS:.proto=.pb.cc)

