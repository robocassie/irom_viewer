# IROM Viewer
=
IROM Viewer is a Nintendo Switch payload that prints the BPMP bootrom onto the screen starting at 0x100000 and allows you to view it in its entirety by pressing volume up and volume down to scroll through it.
It is a modified early version of [prodinfo_gen](https://github.com/CaramelDunes/prodinfo_gen) by CaramelDunes and shchmue, which is based on [Hekate](https://github.com/CTCaer/hekate) by CTCaer.
I made this simply to learn about making a Switch payload and I do not claim to have written nor even understand the bare metal Switch operations code in the original project. I'm still learning, and the folks above are the real geniuses!

![image](image.png)

Building
=
Install [devkitARM](https://devkitpro.org/) and run `make`.
