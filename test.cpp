#include <iostream>
#include <iomanip>
#include <libusb-1.0/libusb.h>

#define PID	0x572b
#define VID 0x0483

using namespace std;

void printdev(libusb_device *dev);

int main(void) {
	libusb_device **devs;
	libusb_context *ctx = nullptr;
	int r;
	size_t cnt;

	if ((r = libusb_init(&ctx)) < 0){
		cerr << "Init Error" << r << endl;
		return -1;
	}

	if((cnt = libusb_get_device_list(ctx, &devs)) <= 0) {
		cerr << "Oops, no devices found" << endl;
		return 0;
	}
	
	cout << "There are " << cnt << " devices." << endl;

	for (size_t i = 0; i < cnt; i++) {
		printdev(devs[i]);
	}

	libusb_exit(ctx);
	return 0;
}

void printconfig(libusb_config_descriptor*);

void printdev(libusb_device *dev) {
	libusb_device_descriptor desc;
	libusb_config_descriptor *config;
	int r;

	if( (r = libusb_get_device_descriptor(dev, &desc)) < 0) {
		cout << "failed to get device descriptor" << endl;
		return;
	}

	// filter for VID/PID 
	if (PID != desc.idProduct || VID != desc.idVendor)
		return;

	cout << "Num configs: " << (int)desc.bNumConfigurations << "   ";
	cout << "Device Class: " << (int)desc.bDeviceClass << "   ";
	cout << "VendorID: " << hex << setw(4) << setfill('0') << desc.idVendor << "   ";
	cout << "ProductID: " << hex << setw(4) << setfill('0') << desc.idProduct << "   ";
	cout << endl;

	if ( (r = libusb_get_config_descriptor(dev, 0, &config)) < 0) {
		cout << "No Configuration Descriptor found" << endl;
		return;
	}

	printconfig(config);
}

void printconfig(libusb_config_descriptor *config) {
	const libusb_interface *iface;
	const libusb_interface_descriptor *iface_desc;
	const libusb_endpoint_descriptor *ep_desc;

	cout << "Interfaces: " << (int)config->bNumInterfaces << endl;
	for (int i=0; i<config->bNumInterfaces; i++) {
		iface = &config->interface[i];
		cout << "Alt settings: " << (int)iface->num_altsetting << endl;
	}
}
