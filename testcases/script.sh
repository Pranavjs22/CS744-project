rm disk*
gcc -I../auxiliary testcase1.c ../auxiliary/emufs-disk.c ../auxiliary/emufs-ops.c
./a.out > output1

gcc -I../auxiliary testcase2.c ../auxiliary/emufs-disk.c ../auxiliary/emufs-ops.c
./a.out > output2
#input a key for encryption

gcc -I../auxiliary testcase3.c ../auxiliary/emufs-disk.c ../auxiliary/emufs-ops.c
./a.out > output3

gcc -I../auxiliary testcase4.c ../auxiliary/emufs-disk.c ../auxiliary/emufs-ops.c
./a.out > output4
# input 2 keys for 2 encrypted devices
