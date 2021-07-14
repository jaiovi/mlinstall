CFLAGS = 
LDFLAGS = -lusb
STYLE = -style=file -i

all: mlinstall clean
gui: gtkb clean

clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam mlinstall *.exe *.zip *.res

# Format files to kernel style
style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

# Build with GTK, and test
GTKFLAGS = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
gtkb:
	$(CC) gtk.c src/*.c $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -o mlinstall
	@./mlinstall

# Build with cli, and test
mlinstall:
	@$(CC) cli.c src/*.c $(CFLAGS) $(LDFLAGS) -o mlinstall
	@sudo ./mlinstall

# Use staticx to convert dynamic to static executable
# pip3 install staticx
static:
	@staticx mlinstall mlinstall

# ------------------------------------------------
# Targets to cross compile for windows, from Linux
# ------------------------------------------------

# Download from https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
# And export top directory to folder "gtk"
# Unzip https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip/download

WINCC = x86_64-w64-mingw32
LIBUSB = libusb-win32-bin-1.2.2.0

# Desired libusb dll directory
LLIBUSB = ../$(LIBUSB)/bin/amd64/libusb0.dll

# win32 + LIBUSB libs
LIB = -lws2_32 -lkernel32 -I$(LIBUSB)/include -I../$(LIBUSB)/include $(LLIBUSB) 

GLIB = -mms-bitfields -I../gtk/include/gtk-3.0 -I../gtk/include/cairo -I../gtk/include/pango-1.0 -I../gtk/include/atk-1.0 -I../gtk/include/cairo -I../gtk/include/pixman-1 -I../gtk/include -I../gtk/include/freetype2 -I../gtk/include/libxml2 -I../gtk/include/freetype2 -I../gtk/include/libpng16 -I../gtk/include/gdk-pixbuf-2.0 -I../gtk/include/libpng16 -I../gtk/include/glib-2.0 -I../gtk/lib/glib-2.0/include -L../gtk/lib -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -lpangocairo-1.0 -lpangowin32-1.0 -lgdi32 -lpango-1.0 -lm -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lglib-2.0
GLIB += ../gtk/bin/libgtk-3-0.dll ../gtk/bin/libgobject-2.0-0.dll

# Download Windows libs (libusb, gtk)
setuplibs:
	@mkdir gtk
	@wget https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
	@unzip gtk+-bundle_3.10.4-20131202_win64.zip -d gtk
	@wget https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	@unzip libusb-win32-bin-1.2.2.0.zip
	@rm *.zip

removelibs:
	@rm -rf gtk libusb-win32-bin-1.2.2.0

windowsgtk:
	@$(WINCC)-windres win.rc -O coff -o win.res
	cd src; $(WINCC)-gcc ../gtk.c ../win.res *.c $(LIB) $(GLIB) $(CFLAGS) -o ../mlinstall.exe

windows:
	cd src; $(WINCC)-gcc ../cli.c *.c $(LIB) $(CFLAGS) -o ../mlinstall.exe

windowsgtkpack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LLIBUSB) ../mlinstall/
	@cp gtk/bin/*.dll mlinstall/
	@cp mlinstall.exe mlinstall/
	@zip -r win64-gui-mlinstall.zip mlinstall

windowspack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LLIBUSB) ../mlinstall/
	@cp mlinstall.exe mlinstall/
	@zip -r win64-cli-mlinstall.zip mlinstall
