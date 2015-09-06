QEMU-QMP
========

Simple test for talking with qemu over qmp to issue 
human monitor commands (registers and cpus)

## Build

    $ make

## Usage

Run qemu with `-qmp` option: `-qmp unix:/tmp/unix.sock,server,nowait`

Example:

    $  qemu-system-x86_64 [-enable-kvm -machine pc-1.3 -cpu host -hda disk.img \
        -net tap,ifname=tun0,script=no -net nic,vlan=0 -smp 2 -m 2048 \
        -gdb tcp::1234 -vnc :0] -qmp unix:/tmp/qmp-sock,server,nowait

Or by providing the following config file 

    [chardev "qmp"]
        backend = "socket"
        path = "/tmp/qmp-sock"
        server = "on"
        wait = "off"
    [mon "qmp"]
        mode = "control"
        chardev = "qmp"
        pretty = "off"

Assuming that the contents above are written into `qemu-qmp.conf` file 
issue `qemu` like this:
    
    $ qemu-system-x86_64 [options] -readconfig qemu-qmp.conf

Start with:

    $ ./qemu-qmp -p /path/to/unix-sock

Then type 'v' or 'r' to display the CPUs or registers.
