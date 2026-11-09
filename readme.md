# Gamdias Hermes P2 Linux HID driver

This out-of-tree HID driver enables the Hermes P2 volume wheel on Linux. The
keyboard is identified as USB vendor/product `1b80:b51d`. Its consumer-control
report declares its input fields as constants, so the generic HID driver
ignores them. This driver enables only the volume-increment and
volume-decrement bits and otherwise uses the standard HID input path.

The driver matches only the Hermes P2. Other keyboards continue to use their
existing HID drivers unchanged.

## Requirements

- Linux 6.x or 7.x, with headers matching the kernel you want to build for.
- A compiler and `make`.
- Root privileges for installation and removal.

## Build and install

```sh
make
sudo make install
```

Installation registers the module with `depmod`, configures it to load before
the generic HID driver, and installs a udev rule that selects it for this
keyboard only. It also rebinds an already-connected Hermes P2. On future
connections the specific driver is selected automatically; no login script or
manual module-loading step is needed.

The default build targets the running kernel. To build (without installing)
for another installed kernel, pass `KERNEL_VERSION=<version>` to `make`. That
kernel's headers must be installed. Run `make clean` before switching target
kernels. Installation is limited to the running kernel and checks that the
module was built for it. Modules may also need to be signed if Secure Boot
module-signature enforcement is enabled.

There is no lock to a particular kernel release, but kernel modules must be
built separately for each kernel. After a kernel update, boot the new kernel
and run `make clean && make` followed by `sudo make install` again. The module
is not automatically rebuilt by DKMS.

## Remove

```sh
sudo make uninstall
```

This returns any connected Hermes P2 interfaces to the generic HID driver,
then removes the installed module and its automatic-load configuration.
During removal the keyboard may briefly disconnect; use another keyboard or
mouse if needed. If removal is interrupted, run `sudo make uninstall` again.

## Development

```sh
make clean
```
