# How to use

## Destination machine
On the destination machine, run the executable with the -destination [dest ip address] command line arguments.

E.g. ".\LazyFileTransfer.exe -destination 192.168.0.2"

The ip address should be the ip address of the destination machine.


## Source Machine
On the source machine, run the executable with the -source [dest ip address] [source filepath] [dest filepath] command line arguments 

E.g. ".\LazyFileTransfer.exe -source 192.168.0.2 "Test.exe" "C:\Test\Test.exe" "C:\LocalFolder\Test2.exe" "C:\Test\blah.exe""

The ip address should also be the ip address of the destination machine

Multiple files can be sent at once, just keep listing them in [source filepath] [dest filepath] format

# How to build

Open the cloned repo's root folder in Visual Studio and the project can easily be built via the presets supplied in CMakePresets.json. 

Compiling for Linux from Windows requires Visual Studio's linux C++ tools as well as WSL.
