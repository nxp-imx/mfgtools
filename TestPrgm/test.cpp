#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libusb.h>
static int count = 0;

int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	static libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor desc;
	int rc;
	(void)libusb_get_device_descriptor(dev, &desc);

	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
	    rc = libusb_open(dev, &handle);
	    if (LIBUSB_SUCCESS != rc) {
		if(rc==LIBUSB_ERROR_ACCESS){
		    printf("no access");
		}
	    printf("Could not open USB device\n");
	    } 
	    else{
		printf("successfully opened device\n");
	    }  
	} 
	else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
	    if (handle) {   
		printf("closed already open device\n");
		libusb_close(handle);
		handle = NULL;
	    }
	    else{

		printf("unplugged unopend device \n");
	    }
	}
	else {
	    printf("Unhandled event %d\n", event);
	}
	count++;
	return 0;
}
int main (void) {
    libusb_hotplug_callback_handle handle;
    int rc;//////////////0781:5406 
    libusb_init(NULL);
    rc = libusb_hotplug_register_callback(NULL,(libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag)0, 0x0781, 0x5406,LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,&handle);
    if (LIBUSB_SUCCESS != rc) {
	printf("Error creating a hotplug callback\n");
	libusb_exit(NULL);
	return EXIT_FAILURE;
    }   
    while (count < 2) {
	libusb_handle_events_completed(NULL, NULL);
	sleep(1);
    }
    libusb_hotplug_deregister_callback(NULL, handle);
    libusb_exit(NULL);


    return 0;
}

