OUTDIR=output
SRCS=windows\debugio_impl.cpp
OBJS=$(SRCS:.cpp=.obj)

build: $(OUTDIR)\example_reader.exe \
       $(OUTDIR)\example_writer.exe

.PHONY: clean
clean:
	del /Q /S *.obj
	del /Q $(OUTDIR)\*

.PHONY: test
test:
	$(OUTDIR)\example_reader.exe | $(OUTDIR)\example_writer.exe

READER_SRCS=example\reader.cpp
READER_OBJS=$(READER_SRCS:.cpp=.obj) $(OBJS)
$(OUTDIR)\example_reader.exe: $(READER_OBJS)
	link.exe /out:"$@" $(READER_OBJS)

WRITER_SRCS=example\writer.cpp
WRITER_OBJS=$(WRITER_SRCS:.cpp=.obj) $(OBJS)
$(OUTDIR)\example_writer.exe: $(WRITER_OBJS)
	link.exe /out:"$@" $(WRITER_OBJS)

.cpp.obj:
    $(CPP) /c /Fo:"$@" $<
