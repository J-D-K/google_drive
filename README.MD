# google_drive
Preview/command line prototype version of JKSV's Google Drive code written in modern-ish C++ using libjson-c and libcurl.

## Usage
Start the application in your terminal of choice and supply the root directory for the local storage being used. Example: `C:/` for Windows or `/home/[user]/Documents` for Linux. Commands work the following ways:
1. First specify the target storage system. Use `local` for your local storage and `drive` for Google Drive followed by one of the following commands:
    1. `list` Prints a list of the files and folders within the current parent directory with their properties.
    2. `chdir [directory name]` Changes the current target/parent directory.
    3. `mkdir [directory name]` Creates a folder in the current parent directory.
    4. `delete [dir/file] [target name]` Deletes the target file or folder.

## Known issues:
Signing in when built under Linux produces a segmentation fault. After the initial sign in, this no longer occurs. Still trying to figure that one out.
