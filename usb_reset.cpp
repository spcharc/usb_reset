#include <iostream>
#include <stdexcept>
#include <libusb-1.0/libusb.h>

std::pair <uint16_t, uint16_t> parse_argv1(const std::string &in) {
    // this function parses argv[1], which is the vendor_id:product_id pair
    // converts them to hexadecimal
    // and returns std::pair <uint16_t, uint16_t>

    if (in.size() != 9 or in[4] != ':')
        throw std::runtime_error(
            "Please provide correct vendor_id:product_id pair.");

    std::string s1 = in.substr(0, 4), s2 = in.substr(5, 4);
    char *ptr;

    unsigned long int num1 = strtoul(s1.c_str(), &ptr, 16);
    if (ptr != s1.c_str() + 4 or num1 >= 65536)
        throw std::runtime_error("Invalid vendor_id");

    unsigned long int num2 = strtoul(s2.c_str(), &ptr, 16);
    if (ptr != s2.c_str() + 4 or num2 >= 65536)
        throw std::runtime_error("Invalid product_id");

    return {num1, num2};
}


// when something went wrong with usblib, this exception is thrown
class UsbLibException: public std::runtime_error {
public:
    UsbLibException(const char *s): std::runtime_error(s) {}
    virtual ~UsbLibException() override {}
};


class DeviceList {
    // this RAII class is used to hold a list of usb devices
    ssize_t cnt;
    libusb_device **devs;   // a list of usb devices

public:
    DeviceList(libusb_context *ctx): cnt(0), devs(nullptr) {
        this->cnt = libusb_get_device_list(ctx, & this->devs);
        if (this->cnt < 0) {
            throw UsbLibException("Failed to get device list from libusb.");
        }
    }

    ~DeviceList() {
        libusb_free_device_list(this->devs, 1);
    }   // 0 = "do not unref any devices", 1 = "unref all devices"

    ssize_t size() const {
        return this->cnt;
    }

    libusb_device *operator[](ssize_t idx) const {
        if (idx < 0 or idx >= this->cnt)
            throw std::out_of_range("DeviceList: Index out of range.");
        return this->devs[idx];
    }
};


class UsbContext {
    // this RAII class is used to hold libusb context
    libusb_context *ctx;

public:
    UsbContext(): ctx(nullptr) {
        if (libusb_init(& this->ctx) != 0)
            throw UsbLibException("Failed to initialize libusb.");
    }

    ~UsbContext() {
        libusb_exit(this->ctx);
    }

    DeviceList get_device_list() const {
        return this->ctx;
    }
};


class DeviceHandle {
    // this RAII class is used to hold a device handle
    libusb_device_handle *handle;

public:
    DeviceHandle(libusb_device *dev): handle(nullptr) {
        int res = libusb_open(dev, & this->handle);
        if (res != 0)
            throw UsbLibException("Failed to get handle for usb device. "
                                  "Insufficient privilege?");
    }

    ~DeviceHandle() {
        libusb_close(this->handle);
    }

    operator libusb_device_handle *() const {
        return this->handle;
    }
};


void reset(const std::pair <uint16_t, uint16_t> &p,
           const char *product = nullptr,
           const char *manufacturer = nullptr) {
    UsbContext ctx;                           // RAII
    DeviceList devs = ctx.get_device_list();  // RAII

    for (int i = 0; i < devs.size(); ++ i) {

        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) != 0)
            throw UsbLibException("Failed to get device descriptor.");

        if (desc.idVendor == p.first && desc.idProduct == p.second) {
            DeviceHandle hnd = devs[i];               // RAII
            unsigned char arr[256];
            char *str = reinterpret_cast <char *> (arr);

            libusb_get_string_descriptor_ascii(
                hnd, desc.iProduct, arr, 256);

            std::string prod(str);

            libusb_get_string_descriptor_ascii(
                hnd, desc.iManufacturer, arr, 256);

            std::string manu(str);

            if ((product and manufacturer and prod != product
                    and manu != manufacturer) or
                (not product and not manufacturer)) {
                std::cout << "Product: " << prod
                          << " (length: " << prod.size() << ')' << std::endl
                          << "Manufacturer: " << manu
                          << " (length: " << manu.size() << ')' << std::endl
                          << "Resetting this device ..." << std::endl;

                if (libusb_reset_device(hnd) != 0)
                    std::cout <<
                        "It seems the reset process did not end properly. "
                        "You should check the device itself to see if it "
                        "succeeded." << std::endl;

                std::cout << "Finished." << std::endl;
            }

            return;
        }
    }
    std::cout << "No such USB device found." << std::endl;
}


int main(int argc, char **argv) {
    if (argc == 2 or argc == 4)
        try {
            if (argc == 2)
                reset(parse_argv1(argv[1]));
            else
                reset(parse_argv1(argv[1]), argv[2], argv[3]);
        } catch (std::exception &s) {
            std::cout << "Error occured." << std::endl
                      << "Detail: " << s.what() << std::endl;
            return 1;
        }
    else
        std::cout << "This program resets a specified USB device based on "
                     "provided\nvendor_id:product_id pair. You can obtain "
                     "this information using \"lsusb\"\ncommand.\nIf you also "
                     "provide the product name and manufacturer name, the "
                     "program will do\na comparison first and reset the device "
                     "only if both of them do NOT match the\nstrings returned "
                     "by the device. (This is because a malfunctioning USB "
                     "device\nthat needs to be resetted usually does not return "
                     "these strings correctly.)\n\nUsage:\n    "
                  << argv[0]
                  << " vendor_id:product_id "
                     "[product_name manufacturer_name]"
                  << std::endl;
    return 0;
}
