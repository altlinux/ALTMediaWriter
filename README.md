# ALT Media Writer

ALT Media Writer is a tool that helps users put ALT images on their portable drives such as flash disks.

It is able to automatically download the required image for them and write them in a `dd`-like fashion, extracting compressed images if necessary.

This overwrites the drive's partition layout though so it also provides a way to restore a single-partition layout with a FAT32 partition.

![ALT Media Writer image list](/dist/screenshots/screenshot1.png)
![ALT Media Writer image details](/dist/screenshots/screenshot2.png)
![ALT Media Writer download dialog](/dist/screenshots/screenshot3.png)

## Troubleshooting

If you experience any problems with the application, like crashes or errors when writing to your drives, please open an issue here on altlinux.space.

## MD5 checksum

Most ALT image files have an associated MD5 checksum for integrity purposes. ALT Media Writer verifies this checksum right after the image is downloaded.
