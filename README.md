# Holiday Widget

Basic startup project for NUCLEO-L053R8 board. 

Source code lives in firmware.

The build directory holds all .o files for compliation. The output binary is build/blink.elf. This folder is generated at compilation time and will not exist upon cloning the repository.

Include holds all user created .h files. For the HAL to work, the stm3l0xx_hal_conf_template.h file needs to be copied and adjusted as necessary. No changes were made as of now. 

src holds all user created .c files. This project does not use the c library so some builtin functions needed to be defined, namely __libc_init_array, memset and memcpy. main.c holds the main function and the basic code to blink the LD2 LED at a different speed when the B1 button is pressed.

linker_script.ld defines how the binary should be setup.  
  
Makefile has two main targets: build/blink.elf and upload  
build/blink.elf is also called by just calling 'make' and creates the .elf file. upload sends the .elf file to the Nucleo board.


The STM32CubeL0 and SmallPrintf folders are submodules for this repo. When cloning the project, run 'git submodule update --init --recursive' to create the folder.


Overall steps to get project and upload to nucleo board:
```
git clone --recursive <repo name> 
git submodule update --init --recursive
make
make upload
```

Dependencies:
* STM32CubeL0 at version v1.12.2
* eyalroz/printf at version v6.2.0-b1
