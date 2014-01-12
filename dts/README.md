Generate DTS from DTB:

	dtc \
		-I dtb \
		-O dts \
		-o ubuntu-`uname -r`.dts \
		/boot/uboot/dtbs/am335x-boneblack.dtb

Enable the PRU.  Change status from "disabled" to "okay"


Generate DTB back from DTS:

	dtc \
		-O dtb \
		-I dts \
		-o /boot/uboot/dtbs/am335x-boneblack.dtb \
		ubuntu-`uname -r`.dts
