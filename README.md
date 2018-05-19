# Network-Disk-Imaging-Protocol
Utility to image disks and transfer via TCP

This project was conceived as I saw the lack of disk imaging utilities available for DOS, particularly in real mode. A tool like this may be useful for anyone wishing to archive old vintage hard drives in machines unable to run tools like WinImage or Clonezilla. In addition, this project will allow a user to transfer the disk image through a TCP connection to a compatible server, saving the disk image on another system. This is useful if you have one hard drive with no means to hold the image, or if breaking the image up into floppies is impractical. This will run in DOS through MTCP. It should be compatible with any hard drive accessible through the BIOS 0x13 interrupt.

