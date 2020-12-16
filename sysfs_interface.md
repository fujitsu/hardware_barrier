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
