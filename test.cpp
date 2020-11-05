#include <iostream>
#include <iomanip>
#include <libusb-1.0/libusb.h>

#define PID	0x572b
#define VID 0x0483

using namespace std;

bool printdev(libusb_device *dev);
bool matchdev(libusb_device *dev);

int main(void) {
	libusb_device **devs{nullptr};
	libusb_context *ctx{nullptr};
	libusb_device_handle *dev{nullptr};
	int r{0};
	size_t cnt{0};
	int idx{-1};

	if ((r = libusb_init(&ctx)) < 0){
		cerr << "Init Error" << r << endl;
		return -1;
	}

	libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

	if((cnt = libusb_get_device_list(ctx, &devs)) <= 0) {
		cerr << "Oops, no devices found" << endl;
		return 0;
	}
	
	cout << "There are " << cnt << " devices." << endl;

	for (int i = 0; i < cnt; i++) {
		if (matchdev(devs[i])) {
			idx = i;
			break;
		}
	}

	if (idx >= 0) {
		cout << "Idx is valid!" << endl;
		printdev(devs[idx]);
	}

	// Okay, so now we know the index of our valid USB HID device.
	if((dev = libusb_open_device_with_vid_pid(ctx, VID, PID)) == nullptr) {
		cout << "Cannot open device" << endl;
		return 0;
	}

	cout << "Device opened!" << endl;

	if(libusb_kernel_driver_active(dev, 0) == 1) {
		if(libusb_detach_kernel_driver(dev, 0) == 0) {
			cout << "Kernel driver detached" << endl;
			return 0;
		}
	}

	libusb_exit(ctx);
	return 0;
}

void printconfig(libusb_config_descriptor*);

bool matchdev(libusb_device *dev) {
	libusb_device_descriptor desc;
	int r;

	if( (r = libusb_get_device_descriptor(dev, &desc)) < 0) {
		cout << "failed to get device descriptor" << endl;
		return false;
	}

	if (PID != desc.idProduct || VID != desc.idVendor)
		return false;
	else
		return true;
}

bool printdev(libusb_device *dev) {
	libusb_device_descriptor desc;
	libusb_config_descriptor *config;
	int r;

	if( (r = libusb_get_device_descriptor(dev, &desc)) < 0) {
		cout << "failed to get device descriptor" << endl;
		return false;
	}

	// filter for VID/PID 
	if (PID != desc.idProduct || VID != desc.idVendor)
		return false;

	cout << "Num configs: " << (int)desc.bNumConfigurations << "   ";
	cout << "Device Class: " << (int)desc.bDeviceClass << "   ";
	cout << "VendorID: " << hex << setw(4) << setfill('0') << desc.idVendor << "   ";
	cout << "ProductID: " << hex << setw(4) << setfill('0') << desc.idProduct << "   ";
	cout << endl;

	if ( (r = libusb_get_config_descriptor(dev, 0, &config)) < 0) {
		cout << "No Configuration Descriptor found" << endl;
		return false;
	}

	printconfig(config);

	return true;
}

void printconfig(libusb_config_descriptor *config) {
	const libusb_interface *iface;
	const libusb_interface_descriptor *iface_desc;
	const libusb_endpoint_descriptor *ep_desc;

	cout << "Interfaces: " << (int)config->bNumInterfaces << ", ";
	for (int i=0; i<config->bNumInterfaces; i++) {
		iface = &config->interface[i];
		cout << "Alt settings: " << (int)iface->num_altsetting << endl;
		for (int j=0; j<iface->num_altsetting; j++) {
			iface_desc = &iface->altsetting[j];
			cout << "    Interface#: " << (int)iface_desc->bInterfaceNumber << " | ";
			cout << "#Endpoints: " << (int)iface_desc->bNumEndpoints << endl;
			for (int k=0; k<iface_desc->bNumEndpoints; k++) {
				ep_desc = &iface_desc->endpoint[k];
				cout << "      Endpoint Type: " << (int)ep_desc->bDescriptorType << " | ";
				cout << "Addr: " << (int)ep_desc->bEndpointAddress << endl;
			}
			cout << endl;
		}
	}
}
