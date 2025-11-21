# CoCo SWD

 ARM Serial Wire Debug (SWD) module for CoCo

## Import
Add coco-swd/\<version> to your conanfile where version corresponds to the git tags

## Features
* Uses coco::Buffer for transferring data to/from device
* Supports block reads and writes for easy transfer of memory contents

## Supported Platforms
* STM32 using SPI
