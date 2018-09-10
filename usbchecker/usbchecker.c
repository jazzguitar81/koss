/*
 * The codes with finding a specific USB device is borrowed from usbtest.c
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>	//access,
#include <ftw.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#define MAX_BUF		100
#define INFO(fmt, args...)	fprintf(stdout, fmt, ## args)
#define DBG(fmt, args...)		\
	if (debug)			\
		fprintf(stdout, fmt, ## args)
#define ERR(fmt, args...)	fprintf(stderr, fmt, ## args)

static int debug = 0;

struct usb_info {
	__u16	idVendor;
	__u16	idProduct;
	int	founded;
};

struct usb_info my_usb;

struct usb_device_descriptor {
	__u8	bLength;
	__u8	bDescriptorType;
	__u16	bcdUSB;
	__u8	bDeviceClass;
	__u8	bDeviceSubClass;
	__u8	bDeivceProtocol;
	__u8	bMaxPacketSize0;
	__u16	idVendor;
	__u16	idProduct;
	__u16	bcdDevice;
	__u8	iManufacturer;
	__u8	iProduct;
	__u8	bNumConfigurations;
} __attribute__ ((packed));

static int hexa_to_decimal(const char *hexadecimal)
{
	int i, j;
	int power = 0;
	int decimal_number = 0;
	char hex_digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
			       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	if (strlen(hexadecimal) != 4) {
		ERR("%s: wrong hexadeciaml number.\n", __func__);
		exit(0);
	}

	for (i = strlen(hexadecimal)-1; i >= 0; i--) {
		for (j = 0; j < 16; j++) {
			if (hexadecimal[i] == hex_digits[j])
				decimal_number += j * pow(16, power);
		}
		power++;
	}

	DBG("[DBG] hexadecimal: 0x%s, decimal: %d\n",
					hexadecimal, decimal_number);
	return decimal_number;
}

static void show_help(void)
{
	printf("./usbchecker -v VID -p PID\n");
	printf("e.g> usbchecker -v 1004 -p 633A\n");
}

static const char *usb_dir_find(void)
{
	static char udev_usb_path[] = "/dev/bus/usb";

	if (access(udev_usb_path, F_OK) == 0)
		return udev_usb_path;

	return NULL;
}

static int find_usb(const char *name, const struct stat *sb, int flag)
{
	int ret;
	FILE *fd;
	struct usb_device_descriptor dev;

	(void)sb;	/* unused */

	if (flag != FTW_F)
		return 0;

	fd = fopen(name, "rb");
	if (!fd) {
		perror(name);
		return 0;
	}

	ret = fread(&dev, sizeof(dev), 1, fd);
	if (ret != 1) {
		perror("fread");
		return -1;
	}

	DBG("name:%s, vid:0x%04x, pid:0x%04x | search-0x%04x:0x%04x\n",
			name, dev.idVendor, dev.idProduct,
			my_usb.idVendor, my_usb.idProduct);

	if (dev.idVendor == my_usb.idVendor &&
	    dev.idProduct == my_usb.idProduct) {
		INFO("Found USB device (VID:0x%x/PID:0x%x)\n",
					dev.idVendor, dev.idProduct);
		my_usb.founded = 1;
	}

	fclose(fd);

	return 0;
}

int main(int argc, char **argv)
{
	int c;

	char *vid = NULL;
	char *pid = NULL;

	const char *usb_dir = NULL;

	if (argc == 1) {
		show_help();
		goto out;
	}

	while ((c = getopt(argc, argv, "hv:p:")) != -1) {
		switch (c) {
		case 'p':
			pid = optarg;
			break;
		case 'v':
			vid = optarg;
			break;
		case 'h':
			show_help();
			return 0;
		default:
			printf("wrong argument\n");
			show_help();
			return -1;
		}
	}

	DBG("[DBG] input: vid-0x%s, pid-0x%s\n", vid, pid);

	my_usb.idVendor = (__u16)hexa_to_decimal(vid);
	my_usb.idProduct = (__u16)hexa_to_decimal(pid);

	/* find usb device subdirectory */
	usb_dir = usb_dir_find();
	if (!usb_dir) {
		ERR("USB device files are missing\n");
		return -1;
	}

	/* check usb with vid and pid */
	if (ftw(usb_dir, find_usb, 3) != 0) {
		ERR("ftw failed: are USB device files missing?\n");
		return -1;
	}

out:
	return 0;
}
