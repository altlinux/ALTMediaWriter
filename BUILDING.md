- [Linux](#linux)
- [Windows](#windows)
  - [Requirements](#requirements)
  - [Build the application](#build-the-application)
  - [Prepare the QtIFW package](#prepare-the-qtifw-package)
    - [Contents of the files](#contents-of-the-files)
  - [Create and test the installer](#create-and-test-the-installer)

# Linux
Build using standard ALTLinux build process.

# Windows
The Windows installer is built with
[Qt Installer Framework](https://doc.qt.io/qtinstallerframework/). The old
MSYS2/Qt 5/NSIS deployment procedure in `dist/win/build.sh` is not used for
Qt 6 packages.

## Requirements

Install [MSYS2](https://www.msys2.org/) on a 64-bit Windows system and open
the **MSYS2 MinGW 64-bit** shell. Do not use the plain MSYS shell. First
update the package database and installed packages:

```bash
pacman -Syu
```

If MSYS2 asks you to close the terminal, close it, start the MinGW 64-bit
shell again and repeat `pacman -Syu`. Then install the build dependencies:

```bash
pacman -S --needed \
    base-devel \
    git \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-7zip \
    mingw-w64-x86_64-qt6-static \
    mingw-w64-x86_64-xz \
    mingw-w64-x86_64-yaml-cpp
```

When `pacman` asks which packages from `mingw-w64-x86_64-toolchain` to
install, press Enter to select the whole group. This installs Qt and the
compiler for the `x86_64` target together with the libraries used directly by
ALT Media Writer. Package dependencies required by Qt are installed by
`pacman` automatically.

Install the following tool separately because it is used to create the final
installer rather than to compile the application:

* [Qt Installer Framework](https://download.qt.io/official_releases/qt-installer-framework/)
  4.8.1 or newer.

Use the same MinGW version and `x86_64` target architecture for Qt, ALT Media
Writer and all third-party libraries. Do not mix 32-bit and 64-bit libraries,
or MinGW and MSVC binaries.

Fetch the repository tags before configuring the project. The build system
uses `git describe` to generate the version shown by the application:

```cmd
git fetch --tags
```

## Build the application

The static Qt package installs `qmake` as
`/mingw64/qt6-static/bin/qmake.exe`. From the repository root, build the
project in the MSYS2 MinGW 64-bit shell:

```bash
mkdir -p build-win64
cd build-win64
/mingw64/qt6-static/bin/qmake.exe .. CONFIG+=release
mingw32-make -j"$(nproc)"
```

Run `mediawriter.exe` from the build directory before preparing the installer
and verify that it can locate `helper.exe` when a write or restore operation is
started.

## Prepare the QtIFW package

The installer source tree must have the following layout:

```text
installer/
|-- config/
|   |-- config.xml
|   `-- mediawriter.ico
`-- packages/
    `-- ru.altlinux.altmediawriter/
        |-- data/
        |   `-- ALTMediaWriter.7z
        `-- meta/
            |-- installscript.qs
            |-- LICENSE.txt
            `-- package.xml
```

`config/mediawriter.ico` is used by QtIFW as the installer icon through
`InstallerApplicationIcon`. No icon is needed in `meta`: the application icon
is embedded into `mediawriter.exe`, and the Start menu shortcut extracts it
from the executable.

The Qt build is static: Qt, QML modules, QML files, translations, images and
the currently used third-party libraries are linked or embedded into the two
executables. Object files, static libraries, generated sources and Makefiles
from `build-win64` are not part of the installer payload. QtIFW receives the
payload as the `ALTMediaWriter.7z` archive rather than as loose executable
files in `data`.

From `build-win64`, stage the two executables, create the archive and copy the
license into the QtIFW package tree:

```bash
SOURCE_ROOT="$(cd .. && pwd -P)"
QFI_DATA="$SOURCE_ROOT/installer/packages/ru.altlinux.altmediawriter/data"
QFI_META="$SOURCE_ROOT/installer/packages/ru.altlinux.altmediawriter/meta"
QFI_STAGE="$(mktemp -d)"
QFI_ARCHIVE="$QFI_DATA/ALTMediaWriter.7z"

mkdir -p "$QFI_DATA" "$QFI_META"
install -m755 app/release/mediawriter.exe "$QFI_STAGE/mediawriter.exe"
install -m755 helper/win/release/helper.exe "$QFI_STAGE/helper.exe"
install -m644 "$SOURCE_ROOT/LICENSE" "$QFI_META/LICENSE.txt"

rm -f "$QFI_ARCHIVE"
(
    cd "$QFI_STAGE"
    7z a -t7z "$QFI_ARCHIVE" mediawriter.exe helper.exe
)
rm -r "$QFI_STAGE"
```

Keep the archive name exactly `ALTMediaWriter.7z`.

Check the archive contents:

```bash
7z l "$QFI_ARCHIVE"
```

The file list must contain only `mediawriter.exe` and `helper.exe` at the
archive root.

Also inspect the runtime imports after every toolchain or dependency change:

```bash
for binary in app/release/mediawriter.exe helper/win/release/helper.exe; do
    printf '%s\n' "$binary"
    objdump -p "$binary" | sed -n 's/.*DLL Name: //p'
done
```

With the current static build this list contains only Windows system DLLs,
such as `KERNEL32.dll`, `USER32.dll`, `ADVAPI32.dll`, `d3d11.dll`,
`DWrite.dll` and `api-ms-win-*.dll`. Do not copy these DLLs into the package.
There should be no Qt or MinGW runtime DLLs such as `Qt6Core.dll`,
`libgcc_s_seh-1.dll`, `libstdc++-6.dll` or `libwinpthread-1.dll`. If a future
change introduces a non-system dependency, locate it with `ldd`, add the DLL
from `/mingw64/bin` beside the executables at the root of
`ALTMediaWriter.7z`, rebuild the archive, and repeat the check for both
executables.

Update the following metadata for every release:

* `Version` in `config/config.xml`;
* `Version` and `ReleaseDate` in `meta/package.xml`;
* the installer icon in `config/mediawriter.ico` when it changes.

### Contents of the files
Below is the contents of the installer files as they are currently used.

`installer/config/config.xml`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Installer>
    <Name>ALT Media Writer</Name>
    <Version>CURRENT_VERSION</Version>
    <Title>AMW</Title>
    <Publisher>ALT</Publisher>
    <StartMenuDir>ALTMediaWriter</StartMenuDir>
<InstallerApplicationIcon>mediawriter</InstallerApplicationIcon>
<TargetDir>@ApplicationsDirX64@\ALT\ALTMediaWriter</TargetDir>
       <AllowSpaceInPath>true</AllowSpaceInPath>
    <!-- <Translations>
        <Translation>ru.qm</Translation>
        <Translation>qt_ru.qm</Translation>
    </Translations> -->
</Installer> 
```

`packages/ru.altlinux.altmediawriter/meta/installscript.qs`:
```qs
function Component() {}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/mediawriter.exe", "@StartMenuDir@/mediawriter.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/mediawriter.exe");
    }
} 
```

`packages/ru.altlinux.altmediawriter/meta/package.xml`:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>ALT Media Writer</DisplayName>
    <Description>The main component</Description>
    <Version>CURRENT_VERSION</Version>
    <ReleaseDate>CURRENT_DATE</ReleaseDate>
    <Default>true</Default>
    <Licenses>
        <License name="GNU GENERAL PUBLIC LICENSE Version 2" file="LICENSE.txt" />
    </Licenses>
    <Script>installscript.qs</Script>
    <Name>ru.altlinux.altmediawriter</Name>
    <ForcedInstallation>true</ForcedInstallation>
</Package> 
```

## Create and test the installer

Run `binarycreator.exe` from the `installer` directory. Include the
architecture in the output file name:

```cmd
C:\Qt\QtIFW-4.8.1\bin\binarycreator.exe -f -c config/config.xml -p packages AMWInstaller.exe 
```

Adjust the QtIFW path to the installed version. `--offline-only` embeds the
package data in the executable and prevents the installer from attempting to
use an online repository.

Test the resulting installer on a clean Windows system that has neither Qt
nor the compiler toolchain installed. Check application startup, the Start
menu shortcut, image downloading, writing and restoring a drive, and
uninstallation.

See also:

* [Creating QtIFW installers](https://doc.qt.io/qtinstallerframework/ifw-creating-installers.html);
* [Creating offline installers](https://doc.qt.io/qtinstallerframework/ifw-offline-installers.html);
* [Qt 6 supported Windows configurations](https://doc.qt.io/qt-6/windows.html);
* [Deploying Qt applications on Windows](https://doc.qt.io/qt-6/windows-deployment.html).
