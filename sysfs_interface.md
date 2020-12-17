# Sysfs interface of hardware barrier driver

Hardware barrier driver creates sysfs interfaces about hardware barrier resources
to ease the debugging of user application. The sysfs interface will be created
upon module load and the location is `/sys/class/misc/fujitsu_hwb`.
All entries is read-only and readable from all user except inti_sync_bb* entry.

## Global entry

There is one global entry.

| name   | meaning |
| -----  | ------- |
| hwinfo | Show number of CMG, number of barrier blade (per CMG), number of barrier window (per CMG) and maximum physical PE number of running system |

## Per CMG entry

Each CMG* folder contains following entries.

| name   | meaning |
| -----  | ------- |
| core_map | List of cpuid and its physical PE number belonging to this CMG |
| used_bb_bmap | Bitmap of currently used barrier blade register in this CMG (in hex)|
| used_bw_bmap | Bitmap of currently used barrier window register per PE in this CMG (in hex)|
| init_sync_bb* | Current value of MASK (first line) and BST field (second line) of INIT_SYNC_BB* register.<br/> MASK and BST field is a bitmap based on the physical PE number (in hex)|

When no resource of hardware barrier is used, the value of used_bb_bmap, used_bw_bmap and
init_sync_bb* should be zero. As access to init_sync_bb* entry may send IPI, read permission
of init_sync_bb* is root-only.

## Example output on FX700

```
# dev, power, subsystem and uevent are automatically created by kernel
$ ls /sys/class/misc/fujitsu_hwb
CMG0  CMG1  CMG2  CMG3  dev  hwinfo  power  subsystem  uevent

# A64FX has 4 CMG, 6 BB per CMG, 4 BW per PE and max 13 PE per CMG
$ cat /sys/class/misc/fujitsu_hwb/hwinfo
4 6 4 13

# Each CMG* folder contains following entries
$ ls /sys/class/misc/fujitsu_hwb/CMG1
core_map  init_sync_bb0  init_sync_bb1  init_sync_bb2  init_sync_bb3  init_sync_bb4  init_sync_bb5  used_bb_bmap  used_bw_bmap

# First column shows cpuid and second column shows its physical PE number
$ cat  /sys/class/misc/fujitsu_hwb/CMG1/core_map
12 0
13 1
14 2
15 3
16 4
17 5
18 6
19 7
20 8
21 9
22 10
23 11

# If no resource is used, it should be zero
# For example, if the third BB is used, 0004 should be read
$ cat  /sys/class/misc/fujitsu_hwb/CMG1/used_bb_bmap
0000

# First column shows cpuid and second column shows currently used BW of it
# If no resource is used, it should be zero
$ cat  /sys/class/misc/fujitsu_hwb/CMG1/used_bw_bmap
12 0000
13 0000
14 0000
15 0000
16 0000
17 0000
18 0000
19 0000
20 0000
21 0000
22 0000
23 0000

# First line is MASK field of INIT_SYNC_BB register and second line is BST filed of it.
# If no resource is used, they should be zero
$ cat  /sys/class/misc/fujitsu_hwb/CMG1/init_sync_bb0
0000
0000
```
