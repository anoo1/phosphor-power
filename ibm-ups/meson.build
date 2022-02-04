ibm_ups = executable(
    'ibm-ups',
    'error_logging.cpp',
    'main.cpp',
    'monitor.cpp',
    'ups.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus
    ],
    install: true
)
