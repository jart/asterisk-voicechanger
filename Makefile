
CFLAGS   = -g -O2 -shared -fPIC -Wall -pedantic --std=gnu99
CXXFLAGS = -g -O2 -shared -fPIC -Wall -pedantic
LDFLAGS  = -lSoundTouch

all: app_voicechanger.so

app_voicechanger.so: app_voicechanger.o voicechanger.o
app_voicechanger.o: app_voicechanger.c
voicechanger.o: voicechanger.cpp

install: all
	cp -av app_voicechanger.so /usr/lib/asterisk/modules/

clean:
	rm -f *.o *.so

%.o: %.c   ; gcc $(CFLAGS)   -c -o $@ $<
%.o: %.cpp ; g++ $(CXXFLAGS) -c -o $@ $<
%.so: %.o  ; gcc $(CFLAGS)      -o $@ $^ $(LDFLAGS)
