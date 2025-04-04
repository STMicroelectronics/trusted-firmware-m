.. _init_level:

#########
Initlevel
#########

Drivers may depend on other drivers being initialized first, or require
the use of System services. All driver using ``SYS_INIT`` or
``DEVICE_DEFINE`` and related allow the user to specify stages
during the boot sequence where the init function will be executed.
Any driver will specify one of five initialization levels:

``EARLY``:

   Used very early in the boot process, no dependencies with system service.
   Can be depend on previous component initialization (ex: debug console).

``PRE_CORE``:

   Used for devices that have no dependencies, such as those that rely on
   hardware present and needed for system (ex clock, reset,)

``CORE``:

   Used for devices that need to register a core service and based only on
   ``PRE_CORE`` dependencies.

``POST_CORE``:

   Used for devices that require system services during configuration.
   At this level the Interrupts are enabled.

``REST``:

   Used for devices can use all system services and not provided a feature at core.

Within each initialization level you may specify a priority level, relative to other
devices in the same initialization level. The priority level is specified as an integer
value in the range 0 to 99; lower values indicate earlier initialization.

.. note::
   On device initialization fail, the device is marked like initialized
   ``dev->state->initialized = true`` and the error code is store in
   ``dev->state->init_res``.

   At the end of init stages, there is no specific strategy: panic, defer (linux)...

   ``device_is_ready`` function return true if device is initialized and no
   error.

.. note::
   The linker script must include ``devices.ld``, a section is created and
   stores objects list sorted by level and priority.
