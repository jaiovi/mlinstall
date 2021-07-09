// GTK+ based GUI app.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>
#include <gtk/gtk.h>

#include "src/config.h"
#include "src/ptp.h"
#include "src/ptpcam.h"

// From ptp.c
uint16_t ptp_runeventproc(PTPParams *params, char string[]);

// flag.c, or flag-win.c
int enableFlag();

int busn = 0, devn = 0;
short force = 0;
PTPParams params;
PTP_USB ptp_usb;
struct usb_device *dev;

// Quick, messy, logging mechanism
GtkWidget *logw;
char logbuf[1000] = "Log info will go here.\n";

void logprint(char string[])
{
	strcat(logbuf, string);
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

void logclear()
{
	logbuf[0] = '\0';
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

// Log a return message after doing something
int returnMessage(unsigned int code)
{
	switch (code) {
	case 0:
		logprint("No PTP/USB device found.");
		return 1;
	}

	char buf[128];
	sprintf(buf, "Response Code: %x\n", code);
	logprint(buf);

	switch (code) {
	case 0x2001:
		logprint("Return Code OK.\n");
		return 0;
	case 0x201d:
		logprint("Return Code INVALID.\n");
		return 1;
	}

	return 0;
}

static void writeflag(GtkWidget *widget, gpointer data)
{
	logclear();
	if (enableFlag()) {
		logprint("Could not enable flags. Make sure\nto run as Administrator/superuser.");
	} else {
		logprint("Wrote card flags on EOS_DIGITAL");
	}
}

static void deviceinfo(GtkWidget *widget, gpointer data)
{
	logclear();
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return;
	}

	PTPDeviceInfo info;
	ptp_getdeviceinfo(&params, &info);

	printf("Manufacturer: %s\n", info.Manufacturer);
	printf("Model: %s\n", info.Model);
	printf("DeviceVersion: %s\n", info.DeviceVersion);
	printf("SerialNumber: %s\n", info.SerialNumber);

	close_camera(&ptp_usb, &params, dev);
}

int runcommand(char string[])
{
	logclear();
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return 1;
	}

	returnMessage((unsigned int)ptp_runeventproc(&params, string));
	close_camera(&ptp_usb, &params, dev);
	return 0;
}

// Run a custom event proc
static void eventproc(GtkWidget *widget, gpointer data)
{
	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(widget));
	runcommand((char *)entry);
}

static void enablebootdisk(GtkWidget *widget, gpointer data)
{
	if (!runcommand("EnableBootDisk")) {
		logprint("Enabled boot disk\n");
	}
}

static void disablebootdisk(GtkWidget *widget, gpointer data)
{
	if (!runcommand("DisableBootDisk")) {
		logprint("Disabled boot disk\n");
	}
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

int main(int argc, char *argv[])
{
	GtkWidget *button;
	GtkWidget *entry;

	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "mlinstall");
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	// Add widgets horizontally
	int order = 0;

	GtkWidget *grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	GtkWidget *title = gtk_label_new(
		"Magic Lantern USB Installation Tools\n" \
		"THIS IS NOT GARUNTEED TO WORK\n" \
		"OR NOT KILL YOUR CAMERA\n" \
		"KEEP BOTH PIECES IF YOU BREAK IT\n"
	);

	gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), title, 0, order++, 1, 1);
	gtk_widget_show(title);

	button = gtk_button_new_with_label("Device Info");
	g_signal_connect(button, "clicked", G_CALLBACK(deviceinfo), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Write card flags on EOS_DIGITAL");
	g_signal_connect(button, "clicked", G_CALLBACK(writeflag), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Enable Boot Disk");
	g_signal_connect(button, "clicked", G_CALLBACK(enablebootdisk), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Disable Boot Disk");
	g_signal_connect(button, "clicked", G_CALLBACK(disablebootdisk), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1);
	gtk_widget_show(button);

	GtkEntryBuffer *buf = gtk_entry_buffer_new("TurnOffDisplay", 14);
	entry = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), entry, 0, order++, 1, 1);
	g_signal_connect(entry, "activate", G_CALLBACK(eventproc), NULL);
	gtk_widget_show(entry);

	logw = gtk_label_new(logbuf);
	gtk_grid_attach(GTK_GRID(grid), logw, 0, order++, 1, 1);
	gtk_label_set_justify(GTK_LABEL(logw), GTK_JUSTIFY_CENTER);
	gtk_widget_show(logw);

	gtk_widget_set_hexpand(button, TRUE);
	gtk_widget_show(grid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
