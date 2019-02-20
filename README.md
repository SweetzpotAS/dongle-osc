# Sweetzpot dongle to OSC bridge

This app will read the data from a dongle and publish it under 
`/breathing`. Each packet contains of:

 * string: MAC address of the device
 * int32: timestamp
 * int32: value

TODO: The value of the timestamp is not set to anything useful.

## Building

You need to have CMake and Make installed.

Make sure you include the submodules:

    git submodule update --init

Compiling:

    mkdir build
    cd build
    cmake ..
    make

Now you can run:

    ./dongle-osc -i [serial port] [ip:port]

On Linux `serial port` will typically be `/dev/ttyUSB0`, on macOS it will typically be
`/dev/cu.SLAB_USBtoUART`

## Testing

This will publish one sample per second from `raw.csv`:

    cat ../raw.csv |\
        while read line; do echo $line; sleep 1; done |\
        ./dongle-osc 127.0.0.1:47120
