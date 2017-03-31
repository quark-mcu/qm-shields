Intel® Quark™ Shield Examples
#############################

Overview
********

The Intel® Quark™ Shield examples are a collection of example applications for
third party shields:

.. contents::

Support
*******

Information and support regarding Intel® Quark™ MCUs can be found in the
following website:

http://www.intel.com/quark/mcu

Hardware Compatibility
**********************

This release has been validated with the following hardware:

* Intel® Quark™ SE Microcontroller C1000 Development Platform.
* Intel® Quark™ Microcontroller D2000 Development Platform.

External Dependencies
*********************

* The ISSM toolchain is required to build the source code. It provides the
  IAMCU toolchain (i586-intel-elfiamcu). The currently supported version is
  "2016-05-12".
* OpenOCD is required to flash the bootloader onto the SoC.

* The toolchain is provided within the ISSM package or
  `standalone tarballs <https://software.intel.com/en-us/articles/issm-toolchain-only-download>`_.

* The sources for qmsi and qm-bootloader can be downloaded from our `github.com
  <https://github.com/quark-mcu/>`_ project page.

License
*******

Please refer to `COPYING <COPYING>`_.

Organization
************
::

	.
	└── grove           : Example applications for seeed grove system

Building
********

The build system is based on the make tool. Make sure you have downloaded the
toolchain as described in `External Dependencies`_.

QMSI sources are needed to build qm-shields code. The QMSI_SRC_DIR environment
variable must point to the QMSI folder, for instance::

  export QMSI_SRC_DIR=$HOME/qmsi

Please read the section `QMSI Building <https://github.com/quark-mcu/qmsi/>`_
of our QMSI github project for more information on additional environment
settings and flashing.
