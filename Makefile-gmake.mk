OUTDIR := output
SRCS   := linux/debugio_impl.cpp
OBJS   := $(SRCS:.cpp=.o)

LIBS = -lpthread -lrt

build: $(OUTDIR)/example_reader \
       $(OUTDIR)/example_writer

.PHONY: clean
clean:
	rm -rf **/*.o
	rm -rf $(OUTDIR)/*

.PHONY: test
test:
	$(OUTDIR)/example_reader | $(OUTDIR)/example_writer

READER_SRCS := example/reader.cpp
READER_OBJS := $(READER_SRCS:.cpp=.o) $(OBJS)
$(OUTDIR)/example_reader: $(READER_OBJS)
	$(CXX) -o $@ $^ $(LIBS)

WRITER_SRCS := example/writer.cpp
WRITER_OBJS := $(WRITER_SRCS:.cpp=.o) $(OBJS)
$(OUTDIR)/example_writer: $(WRITER_OBJS)
	$(CXX) -o $@ $^ $(LIBS)

.cpp.obj:
	$(CXX) -g -O0 -fstack-protector -c $<
