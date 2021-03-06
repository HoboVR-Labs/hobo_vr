# HoboVR
The compiled driver that SteamVR loads to connect to headsets.

## dev version setup
There will be 2 scripts present in the dev builds of this driver,
`register_driver.sh`(or `.bat` depending on the platform) and `unregister_driver`(or `.bat` depending on the platform)

Run `register_driver.sh`(or `.bat` depending on the platform) to register the driver with SteamVR

Run `unregister_driver.sh`(or `.bat` depending on the platform) to unregister the driver with SteamVR

## bin
Contains the compiled driver. Both 32 bit and 64 bit drivers are needed.

## resources
Contains the VR icons, 3D models, settings to point SteamVR to the right icons or 3D models (in driver.vrresources), input bindings, and general settings.

`resources/settings/default.vrsettings` has the default settings that would more often be changed for different HMDs, like IPD, resolution, frequency, etc.

## driver.vrdrivermanifest
Should never be changed. It includes things like the unique name of the driver.
