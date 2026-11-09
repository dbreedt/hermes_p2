#include <linux/hid.h>
#include <linux/module.h>

#define USB_VENDOR_ID_AFATECH 0x1b80
#define USB_DEVICE_ID_GAMDIAS_HERMES_P2 0xb51d
#define HERMES_P2_CONSUMER_REPORT_ID 2
#define HID_USAGE_PAGE_CONSUMER 0x0c
#define HERMES_P2_EXPECTED_VOLUME_USAGES 2
#define HERMES_P2_EXPECTED_INPUT_FIELDS 1

static int hermes_p2_fix_consumer_inputs(__u8 *rdesc, unsigned int rsize,
					 bool patch, bool *report_found,
					 unsigned int *volume_usages)
{
	unsigned int offset = 0;
	unsigned int count = 0;
	unsigned int usage_page = 0;
	unsigned int report_id = 0;
	bool first_input_seen = false;

	while (offset < rsize) {
		__u8 prefix = rdesc[offset];
		unsigned int item_size;
		unsigned int item_type;
		unsigned int item_tag;
		unsigned int value = 0;
		unsigned int i;

		if (prefix == 0xfe) {
			if (rsize - offset < 3)
				return -EINVAL;
			item_size = rdesc[offset + 1];
			if (rsize - offset < item_size + 3)
				return -EINVAL;
			offset += item_size + 3;
			continue;
		}

		item_size = prefix & 0x03;
		if (item_size == 0x03)
			item_size = 4;
		if (rsize - offset < item_size + 1)
			return -EINVAL;

		item_type = (prefix >> 2) & 0x03;
		item_tag = prefix >> 4;
		for (i = 0; i < item_size; i++)
			value |= (unsigned int)rdesc[offset + 1 + i] << (8 * i);

		if (item_type == 1) {
			if (item_tag == 0)
				usage_page = value;
			else if (item_tag == 8)
				report_id = value;
		}

		if (report_id == HERMES_P2_CONSUMER_REPORT_ID &&
		    usage_page == HID_USAGE_PAGE_CONSUMER) {
			*report_found = true;
			if (!first_input_seen && item_type == 2 && item_tag == 0) {
				if (value == 0x00e9 || value == 0x00ea) {
					(*volume_usages)++;
				} else if (patch) {
					for (i = 0; i < item_size; i++)
						rdesc[offset + 1 + i] = 0;
				}
			}

			if (item_type == 0 && item_tag == 8) {
				if (!first_input_seen && item_size == 1 &&
				    value == 0x03) {
					count++;
					if (patch)
						rdesc[offset + 1] = 0x02;
				}
				first_input_seen = true;
			}
		}

		offset += item_size + 1;
	}

	return count;
}

/* The callback's return type changed from __u8 * to const __u8 * in Linux 6.x. */
typedef typeof(((struct hid_driver *)0)->report_fixup(NULL, NULL, NULL))
	hermes_p2_fixup_result;

static hermes_p2_fixup_result hermes_p2_report_fixup(struct hid_device *hdev,
						     __u8 *rdesc,
						     unsigned int *rsize)
{
	bool report_found = false;
	unsigned int volume_usages = 0;
	int input_fields;

	input_fields = hermes_p2_fix_consumer_inputs(rdesc, *rsize, false,
						     &report_found,
						     &volume_usages);
	if (input_fields < 0) {
		hid_warn(hdev, "malformed HID report descriptor; not applying fixup\n");
		return rdesc;
	}

	if (!report_found)
		return rdesc;

	if (input_fields != HERMES_P2_EXPECTED_INPUT_FIELDS ||
	    volume_usages != HERMES_P2_EXPECTED_VOLUME_USAGES) {
		hid_warn(hdev, "unexpected consumer report layout; not applying fixup\n");
		return rdesc;
	}

	report_found = false;
	volume_usages = 0;
	hermes_p2_fix_consumer_inputs(rdesc, *rsize, true, &report_found,
				      &volume_usages);
	hid_info(hdev, "enabled the keyboard's volume wheel controls\n");

	return rdesc;
}

static int hermes_p2_probe(struct hid_device *hdev,
			   const struct hid_device_id *id)
{
	int ret;

	ret = hid_parse(hdev);
	if (ret)
		return ret;

	return hid_hw_start(hdev, HID_CONNECT_DEFAULT);
}

static void hermes_p2_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);
}

static const struct hid_device_id hermes_p2_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_AFATECH,
			 USB_DEVICE_ID_GAMDIAS_HERMES_P2) },
	{ }
};
MODULE_DEVICE_TABLE(hid, hermes_p2_devices);

static struct hid_driver hermes_p2_driver = {
	.name = "hid_gamdias_hermes_p2",
	.id_table = hermes_p2_devices,
	.probe = hermes_p2_probe,
	.remove = hermes_p2_remove,
	.report_fixup = hermes_p2_report_fixup,
};
module_hid_driver(hermes_p2_driver);

MODULE_DESCRIPTION("HID support for the Gamdias Hermes P2 keyboard");
MODULE_AUTHOR("OpenAI");
MODULE_LICENSE("GPL");
