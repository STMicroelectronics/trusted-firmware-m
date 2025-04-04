.. _devicetree_tfm:

#################
Scope and purpose
#################

The device tree (DT) allows to describe hardware available on supported boards.
This desciption defines inter dependencies between devices.

Usually, the DT representation is created and passed to the kernel as a binary
blob (dtb). At runtime the kernel uses a library to parse this blob to look up
the hardware informations. Like this, a kernel can be independant
of hardware description and same kenel can be compatible with several boards, SoC...

However, this way has 2 disadvantages for small system:

* Increase embedded image size, which integrate parsing library and the blob.
* Increase boot time: each device looks up in blob

In 2016, Zephyr offers an alternative for small system, which uses the advantage of
DT description whitout parse a blob. TF-M is based on this alternative.

#######
Concept
#######

There are two types of devicetree input files: devicetree sources (dts, dtsi) and devicetree bindings (yaml).
The sources contain the devicetree itself. The bindings describe its contents, including
data types. The build system uses devicetree sources and bindings to produce a generated C header.

.. figure:: tfm_dt_build.png
   :figclass: align-center

To simplify, gen_defines.py script creates for each information a define with specific name and its value.

.. code-block:: devicetree

   / {
	first_node {
		second_node_label: second_node@10000000 {
			foo-val = <3>;
                        status = "okay";
		};
        };
     };

Example of define's generated

.. code-block:: c

        #define DT_N_S_first_node_S_second_node_10000000_PATH "/first_node/second_node@10000000"
        #define DT_N_S_first_node_S_second_node_10000000_P_compatible {"foo,foo-compatible"}
        #define DT_N_S_first_node_S_second_node_10000000_STATUS_okay 1
        #define DT_N_S_first_node_S_second_node_10000000_P_foo_val 3

The name is built from the devicetree path and word key like (list is not exhaustive):

* ``DT_N`` header for devicetree node
* ``S`` for ``/``
* ``P`` for property

To easily exploit the generated define, some macros are available in "devicetree.h" or framework include file (clk.h..).
All macro names start with ``DT_``.

Example:

.. code-block:: c

        // return node's label property value
        #define DT_LABEL(node_id)
        // return 1 if the node has the property, 0 otherwise.
        #define DT_NODE_HAS_PROP(node_id, prop)
        // return a representation of the property's value
        #define DT_PROP(node_id, prop)
        // return node's register block address
        #define DT_REG_ADDR(node_id)

#################
How to add device
#################

In this section we would to add and enable device using a new driver that requires some ressources
(reg, int value).

************************************
Devicetree (dts) and bindings (yaml)
************************************

Files location:

- the devicetree source (dts, dtsi): ``devicetree/dts/<soc-family>/<vendor>``
- bindings (yaml): ``devicetree/bindings/<framwork>/``

.. code-block:: devicetree

   /* Node in a DTS file */
   my-device@10000000 {
     compatible = "foo-company,my-device";
     reg = <0x10000000 0x400>;
     num-foos = <3>;
     status = "okay";
   };

.. code-block:: yaml

   description: my-device description

   compatible: "foo-company,my-device"

   include: [base.yaml]

   properties:
      reg:
         required: true

      num-foos:
         type: int
         description: integer value

By the ``compatible`` field, the build system matches ``my-device`` node to its yaml file.
Yaml file allows to check the ``properties:`` (type, required) and generates define adapted
to type.

Bindings can include other files, which can be used to share common property definitions between bindings.
Use the ``include:`` key for this. Its value is either a string or a list.

*******
Drivers
*******

.. code-block:: c

   // define compatible value, needed for DT_INST_XX macro
   #define DT_DRV_COMPAT foo_company_my_device

   // include generic device api and devicetree
   #include <device.h>
   #include <debug.h>

   struct my_device_config {
	uintptr_t base;
	uint32_t num_foos;
   };

   struct my_device_data {
   };

   int my_device_init(const struct device *dev)
   {
	const struct my_device_config *dev_cfg = dev_get_config(dev);

	IMSG("my property num-foos:%d", dev_cfg->num_foos);

	return 0;
   }

   #define MY_DEVICE_INIT(n)					\
								\
   static const struct my_device_config my_dev_cfg_##n = {	\
    .base = DT_INST_REG_ADDR(n),				\
    .num_foos = DT_INST_PROP_OR(n, num_foos, 0),		\
   };								\
								\
   static struct my_device_data my_dev_data_##n = {		\
   };								\
								\
   DEVICE_DT_INST_DEFINE(n,					\
	      &my_device_init,					\
	      &my_dev_data_##n,					\
	      &my_dev_cfg_##n,					\
	      POST_CORE, 10,					\
	      /*&fw_controller_api*/ NULL);

   DT_INST_FOREACH_STATUS_OKAY(MY_DEVICE_INIT)

For each instances with compatible ``foo-company,my-device`` and status
``okay`` a device structure is defined with:

* Pointer to the device’s initialization function, which will be call during system initialization.
* A private data structure by instance.
* A config structure (constant) by instance.
* The device’s initialization level (see :ref:`init_level`).
* A framework API, if this controller is under a framework.

.. note::

   Tips to debug this part:

   * Check devicetree source (dts) preprocessing in
     ``<BUILD_PATH>/generated/devicetree/<tfm_s|tfm_ns|bl2>`` files ``*.dts.pre.tmp`` or ``out.dts``
   * Check if ``devicetree_generated.h`` contains your device and its properties
   * Preprocess your c file to check if your device instance is filled with devicetree information

   .. code-block:: console

      make -f platform/CMakeFiles/platform_s.dir/build.make platform/CMakeFiles/platform_s.dir/ext/target/<DRIVER_PATH>/foo.i

--------------

*Copyright (c) 2025 STMicroelectronics. All rights reserved.*
*SPDX-License-Identifier: BSD-3-Clause*
