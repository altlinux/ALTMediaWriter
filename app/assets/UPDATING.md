# How to update assets

## Update logos

Update "i/logo" folder to match the same folder from getalt.

Also update logo paths in "assets.qrc". Add/update/remove entries in the qrc file so that all needed files are present there. "file alias" must match the "img" attribute of the release in the sections file. In general, that would be the same as the filename but currently there's an exception for "alt-server" having a logo named "arm" because of an error in the sections file.

## Troubleshooting
Run the app. Check for any errors or warnings to make sure that assets are setup correctly.
