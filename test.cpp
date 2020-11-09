#include <iostream>
#include <iomanip>
#include <libusb-1.0/libusb.h>

#define VID 0x0483
#define PID 0x572b
//#define VID 0x068e
//#define PID 0x00f3

// Polling interval: in USB frame units, 1ms for SS|FS, 125us for HS

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
	bool hadKernelDriver{false};

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
			cout << "Found the one we want!" << endl;
			break;
		}
	}

	while (idx) {
		cout << "Idx is valid!" << endl;
		printdev(devs[idx]);

		// Okay, so now we know the index of our valid USB HID device.
		if((dev = libusb_open_device_with_vid_pid(ctx, VID, PID)) == nullptr) {
			cout << "Cannot open device" << endl;
			break;
		}

		cout << "Device opened!" << endl;

		if(libusb_kernel_driver_active(dev, 0) == 1) {
			if(libusb_detach_kernel_driver(dev, 0) == 0) {
				cout << "Kernel driver detached" << endl;
				hadKernelDriver = true;
			}
			else
				break;
		}

		if ((r = libusb_claim_interface(dev, 0)) < 0) {
			cout << "Can't claim interface" << endl;
			break;
		}

		cout << "Interface claimed!" << endl;

		// so, at this point I *should* be able to read from the device.
		while(0) {
			unsigned char inputBuffer[6]{0};
			int len{0};
			int recv_ret = libusb_interrupt_transfer(dev, 0x81, inputBuffer, sizeof(inputBuffer), &len, 0);
			cout << "Report: " 
				<< to_string(inputBuffer[0]) 
				<< "," 
				<< to_string(inputBuffer[1]) 
				<< "," 
				<< to_string(inputBuffer[2]) 
				<< "," 
				<< to_string(inputBuffer[3]) 
				<< "," 
				<< to_string(inputBuffer[4]) 
				<< "," 
				<< to_string(inputBuffer[5]) 
				<< endl;
		}
		idx = 0;
	}

	if (hadKernelDriver)
		libusb_attach_kernel_driver(dev, 0);

	if (dev)
		libusb_close(dev);

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
	const string transferTypeStrings[] = {"Control", "Isochronous", "Bulk", "Interrupt"};
	const string syncTypeStrings[] = {"None", "Asynchronous", "Adaptive", "Synchronous"};
	const string transferDirStrings[] = {"Output", "Input"};

	cout << "Interfaces: " << (int)config->bNumInterfaces << ", ";
	for (int i=0; i<config->bNumInterfaces; i++) {
		iface = &config->interface[i];
		cout << "Alt settings: " << (int)iface->num_altsetting << endl;
		for (int j=0; j<iface->num_altsetting; j++) {
			iface_desc = &iface->altsetting[j];
			cout << "  Interface: " << (int)iface_desc->bInterfaceNumber << " | ";
			cout << "Endpoints: " << (int)iface_desc->bNumEndpoints << endl;
			for (int k=0; k<iface_desc->bNumEndpoints; k++) {
				ep_desc = &iface_desc->endpoint[k];
				cout << "    Endpoint Addr: " << to_string(ep_desc->bEndpointAddress & 0xf) << " | ";
				auto transferDir = (ep_desc->bEndpointAddress & 0x80) >> 7;
				cout << "Dir: " << transferDirStrings[transferDir] << " | ";
				auto transferType = (ep_desc->bmAttributes & 0x3);
				cout << "Type: " << transferTypeStrings[transferType] << " | ";
				if (transferType == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
					auto syncType = (ep_desc->bmAttributes & 0x0C) >> 2;
					cout << "Sync: " << syncTypeStrings[syncType] << " | ";
				}
				cout << "MaxPacketSize: " << to_string(ep_desc->wMaxPacketSize) << " | ";
				cout << "Interval: " << to_string(ep_desc->bInterval) << " | ";
				cout << "Refresh: " << to_string(ep_desc->bRefresh) << " | ";
				cout << "SyncAddress: " << to_string(ep_desc->bSynchAddress) << " | ";
				cout << "ExtraLength: " << to_string(ep_desc->extra_length);
				cout << endl;
				
			}
			if (iface_desc->extra_length > 0) {
				cout << "Extra Length: " << to_string(iface_desc->extra_length) << endl;
				for (int i=0; i<iface_desc->extra_length; i++) {
					printf("0x%02X, ", iface_desc->extra[i] & 0xff);
				}
				cout << endl;
			}
		}
	}
}
