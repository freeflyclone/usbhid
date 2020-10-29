PROGRAM=test
LIBUSB_LIBS=$(shell pkg-config libusb-1.0 --libs)


$(PROGRAM): test.o
	$(CXX) -o $@ $< $(LIBUSB_LIBS)

foo: foo.o
	$(CXX) -o $@ $<

clean:
	@-rm *.o > /dev/null 2>&1
	@-rm $(PROGRAM) foo > /dev/null 2>&1
