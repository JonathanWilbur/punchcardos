/* This is a tool that formats a file as a minimally viable ext2 partition. */
#include <linux/types.h>
#ifndef NOLIBC
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#endif

#define ROOT_UID        0
#define ROOT_GID        0

#define GROUPS_COUNT        1

#define EXT2_BAD_INO	        1	// bad blocks inode
#define EXT2_ROOT_INO	        2	// root directory inode
#define EXT2_ACL_IDX_INO	    3	// ACL index inode (deprecated?)
#define EXT2_ACL_DATA_INO	    4	// ACL data inode (deprecated?)
#define EXT2_BOOT_LOADER_INO	5	// boot loader inode
#define EXT2_UNDEL_DIR_INO	    6	// undelete directory inode

#define EXT2_MAJOR_VERS         0
#define EXT2_MINOR_VERS         0

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-nonstring-variable-attribute
#ifndef __nonstring
#ifdef __has_attribute
#if __has_attribute(__nonstring__)
#define __nonstring                    __attribute__((__nonstring__))
#else
#define __nonstring
#endif /* __has_attribute(__nonstring__) */
#else
# define __nonstring
#endif /* __has_attribute */
#endif /* __nonstring */

#define EXT2_LABEL_LEN				16
#define EXT2_BLOCK_SIZE             1024
#define SUPERBLOCK_OFFSET           1024
#define EXT2_SUPER_MAGIC            0xEF53
#define INODE_TABLE_OFFSET          (2 * EXT2_BLOCK_SIZE)
#define S_FIRST_DATA_BLOCK          (EXT2_BLOCK_SIZE == 1024 ? 1 : 0)

/*
 * Constants relative to the data blocks
 */
#define EXT2_NDIR_BLOCKS		12
#define EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255

/*
 * Ext2/linux mode flags.  We define them here so that we don't need
 * to depend on the OS's sys/stat.h, since we may be compiling on a
 * non-Linux system.
 */
#define LINUX_S_IFMT  00170000
#define LINUX_S_IFSOCK 0140000
#define LINUX_S_IFLNK	 0120000
#define LINUX_S_IFREG  0100000
#define LINUX_S_IFBLK  0060000
#define LINUX_S_IFDIR  0040000
#define LINUX_S_IFCHR  0020000
#define LINUX_S_IFIFO  0010000
#define LINUX_S_ISUID  0004000
#define LINUX_S_ISGID  0002000
#define LINUX_S_ISVTX  0001000

/*
 * File system states
 */
#define EXT2_VALID_FS			0x0001	/* Unmounted cleanly */
#define EXT2_ERROR_FS			0x0002	/* Errors detected */
#define EXT3_ORPHAN_FS			0x0004	/* Orphans being recovered */
#define EXT4_FC_REPLAY			0x0020	/* Ext4 fast commit replay ongoing */

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_CONTINUE		1	/* Continue execution */
#define EXT2_ERRORS_RO			2	/* Remount fs read-only */
#define EXT2_ERRORS_PANIC		3	/* Panic */
#define EXT2_ERRORS_DEFAULT		EXT2_ERRORS_CONTINUE

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX		0
#define EXT2_OS_HURD		1
#define EXT2_OBSO_OS_MASIX	2
#define EXT2_OS_FREEBSD		3
#define EXT2_OS_LITES		4

/*
 * Structure of the super block
 */
struct ext2_super_block {
    __u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
	__u32	s_free_blocks_count;	/* Free blocks count */
    __u32	s_free_inodes_count;	/* Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
	__u32	s_log_block_size;	/* Block size */
	__u32	s_log_cluster_size;	/* Allocation cluster size */
    __u32	s_blocks_per_group;	/* Number of Blocks per group */
	__u32	s_clusters_per_group;	/* Number of Fragments per group */
	__u32	s_inodes_per_group;	/* Number of Inodes per group */
	__u32	s_mtime;		/* Mount time */
    __u32	s_wtime;		/* Write time */
	__u16	s_mnt_count;		/* Mount count since consistency check */
	__s16	s_max_mnt_count;	/* Maximal mount count before consistency check*/
	__u16	s_magic;		/* Magic signature */
	__u16	s_state;		/* File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level;	/* minor revision level */
    __u32	s_lastcheck;		/* time of last check */
	__u32	s_checkinterval;	/* max. time between checks */
	__u32	s_creator_os;		/* OS */
	__u32	s_rev_level;		/* Revision level */
    __u16	s_def_resuid;		/* Default uid for reserved blocks */
	__u16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__u32	s_first_ino;		/* First non-reserved inode */
	__u16   s_inode_size;		/* size of inode structure */
	__u16	s_block_group_nr;	/* block group # of this superblock */
	__u32	s_feature_compat;	/* compatible feature set */
    __u32	s_feature_incompat;	/* incompatible feature set */
	__u32	s_feature_ro_compat;	/* readonly-compatible feature set */
    __u8	s_uuid[16] __nonstring;		/* 128-bit uuid for volume */
    __u8	s_volume_name[EXT2_LABEL_LEN] __nonstring;	/* volume name, no NUL? */
    __u8	s_last_mounted[64] __nonstring;	/* directory last mounted on, no NUL? */
    __u32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_reserved_gdt_blocks;	/* Per group table for online growth */
	/*
	 * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
    __u8	s_journal_uuid[16] __nonstring;	/* uuid of journal superblock */
    __u32	s_journal_inum;		/* inode number of journal file */
	__u32	s_journal_dev;		/* device number of journal file */
	__u32	s_last_orphan;		/* start of list of inodes to delete */
    __u32	s_hash_seed[4];		/* HTREE hash seed */
    __u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_jnl_backup_type;	/* Default type of journal backup */
	__u16	s_desc_size;		/* Group desc. size: INCOMPAT_64BIT */
    __u32	s_default_mount_opts;	/* default EXT2_MOUNT_* flags used */
	__u32	s_first_meta_bg;	/* First metablock group */
	__u32	s_mkfs_time;		/* When the filesystem was created */
    __u32	s_jnl_blocks[17];	/* Backup of the journal inode */
    __u32	s_blocks_count_hi;	/* Blocks count high 32bits */
	__u32	s_r_blocks_count_hi;	/* Reserved blocks count high 32 bits*/
	__u32	s_free_blocks_hi;	/* Free blocks count */
	__u16	s_min_extra_isize;	/* All inodes have at least # bytes */
	__u16	s_want_extra_isize;	/* New inodes should reserve # bytes */
    __u32	s_flags;		/* Miscellaneous flags */
	__u16	s_raid_stride;		/* RAID stride in blocks */
	__u16	s_mmp_update_interval;  /* Number of seconds to wait in MMP checking */
	__u64	s_mmp_block;		/* Block for multi-mount protection */
    __u32	s_raid_stripe_width;	/* blocks on all data disks (N*stride)*/
	__u8	s_log_groups_per_flex;	/* FLEX_BG group size */
	__u8	s_checksum_type;	/* metadata checksum algorithm */
	__u8	s_encryption_level;	/* versioning level for encryption */
	__u8	s_reserved_pad;		/* Padding to next 32bits */
	__u64	s_kbytes_written;	/* nr of lifetime kilobytes written */
    __u32	s_snapshot_inum;	/* Inode number of active snapshot */
	__u32	s_snapshot_id;		/* sequential ID of active snapshot */
	__u64	s_snapshot_r_blocks_count; /* active snapshot reserved blocks */
    __u32	s_snapshot_list;	/* inode number of disk snapshot list */
#define EXT4_S_ERR_START ext4_offsetof(struct ext2_super_block, s_error_count)
	__u32	s_error_count;		/* number of fs errors */
	__u32	s_first_error_time;	/* first time an error happened */
	__u32	s_first_error_ino;	/* inode involved in first error */
    __u64	s_first_error_block;	/* block involved in first error */
	__u8	s_first_error_func[32] __nonstring;	/* function where error hit, no NUL? */
    __u32	s_first_error_line;	/* line number where error happened */
	__u32	s_last_error_time;	/* most recent time of an error */
    __u32	s_last_error_ino;	/* inode involved in last error */
	__u32	s_last_error_line;	/* line number where error happened */
	__u64	s_last_error_block;	/* block involved of last error */
    __u8	s_last_error_func[32] __nonstring;	/* function where error hit, no NUL? */
#define EXT4_S_ERR_END ext4_offsetof(struct ext2_super_block, s_mount_opts)
    __u8	s_mount_opts[64] __nonstring;	/* default mount options, no NUL? */
    __u32	s_usr_quota_inum;	/* inode number of user quota file */
	__u32	s_grp_quota_inum;	/* inode number of group quota file */
	__u32	s_overhead_clusters;	/* overhead blocks/clusters in fs */
    __u32	s_backup_bgs[2];	/* If sparse_super2 enabled */
    __u8	s_encrypt_algos[4];	/* Encryption algorithms in use  */
    __u8	s_encrypt_pw_salt[16];	/* Salt used for string2key algorithm */
    __le32	s_lpf_ino;		/* Location of the lost+found inode */
	__le32  s_prj_quota_inum;	/* inode for tracking project quota */
    __le32	s_checksum_seed;	/* crc32c(orig_uuid) if csum_seed set */
    __u8	s_wtime_hi;
	__u8	s_mtime_hi;
	__u8	s_mkfs_time_hi;
	__u8	s_lastcheck_hi;
	__u8	s_first_error_time_hi;
	__u8	s_last_error_time_hi;
	__u8	s_first_error_errcode;
	__u8    s_last_error_errcode;
    __le16	s_encoding;		/* Filename charset encoding */
	__le16	s_encoding_flags;	/* Filename charset encoding flags */
	__le32  s_orphan_file_inum;	/* Inode for tracking orphan inodes */
	__le32	s_reserved[94];		/* Padding to the end of the block */
    __u32	s_checksum;		/* crc32c(superblock) */
};

/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
/*00*/	__u16	i_mode;		/* File mode */
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u32	i_size;		/* Size in bytes */
	__u32	i_atime;	/* Access time */
	__u32	i_ctime;	/* Inode change time */
/*10*/	__u32	i_mtime;	/* Modification time */
	__u32	i_dtime;	/* Deletion Time */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u16	i_links_count;	/* Links count */
	__u32	i_blocks;	/* Blocks count */
/*20*/	__u32	i_flags;	/* File flags */
	union {
		struct {
			__u32	l_i_version; /* was l_i_reserved1 */
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
	} osd1;				/* OS dependent 1 */
/*28*/	__u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
/*64*/	__u32	i_generation;	/* File version (for NFS) */
	__u32	i_file_acl;	/* File ACL */
	__u32	i_size_high;
/*70*/	__u32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u16	l_i_blocks_hi;
			__u16	l_i_file_acl_high;
			__u16	l_i_uid_high;	/* these 2 fields    */
			__u16	l_i_gid_high;	/* were reserved2[0] */
			__u16	l_i_checksum_lo; /* crc32c(uuid+inum+inode) */
			__u16	l_i_reserved;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
	} osd2;				/* OS dependent 2 */
};

/*
 * Structure of a directory entry
 */
struct ext2_dir_entry {
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u16	name_len;		/* Name length */
	char	name[EXT2_NAME_LEN];	/* File name */
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	__u32	bg_block_bitmap;	/* Blocks bitmap block */
	__u32	bg_inode_bitmap;	/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_flags;
	__u32	bg_exclude_bitmap_lo;	/* Exclude bitmap for snapshots */
	__u16	bg_block_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	__u16	bg_inode_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bitmap) LSB */
	__u16	bg_itable_unused;	/* Unused inodes count */
	__u16	bg_checksum;		/* crc16(s_uuid+group_num+group_desc)*/
};

#define INODES_PER_GROUP            1024
// Since each bitmap is limited to a single block, this means that the maximum size of a block group is 8 times the size of a block.
#define BLOCKS_PER_GROUP            EXT2_BLOCK_SIZE * 8
#define FRAGS_PER_GROUP             EXT2_BLOCK_SIZE * 8

// WARNING: Make sure that INODES_PER_GROUP and BLOCKS_PER_GROUP are evenly divisible by 8!
#define BYTES_FOR_BLOCK_BITMAP      (BLOCKS_PER_GROUP >> 3)
#define BLOCKS_FOR_BLOCK_BITMAP     (BYTES_FOR_BLOCK_BITMAP / EXT2_BLOCK_SIZE)

#define BYTES_FOR_INODE_BITMAP      (INODES_PER_GROUP >> 3)
#define BLOCKS_FOR_INODE_BITMAP     (BYTES_FOR_INODE_BITMAP / EXT2_BLOCK_SIZE)

#define BYTES_FOR_INODE_TABLE       (INODES_PER_GROUP * INODE_SIZE)
#define BLOCKS_FOR_INODE_TABLE      (BYTES_FOR_INODE_TABLE / EXT2_BLOCK_SIZE)

// This is ONLY a fixed value if version < 1.
#define INODE_SIZE  128

#define INODES_COUNT        (GROUPS_COUNT * INODES_PER_GROUP)

int write_empty_block (int ofd) {
    char buf[EXT2_BLOCK_SIZE];
    memset(&buf, 0, sizeof(buf));
    return write(ofd, &buf, sizeof(buf));
}

int write_empty_inode (int ofd) {
    struct ext2_inode inode;
    memset(&inode, 0, sizeof(inode));
    return write(ofd, &inode, sizeof(inode));
}

// Unfortunately, this seems to be how you do this.
int write_padding (int ofd, int size) {
    char *padbuf = malloc(size);
    if (padbuf == NULL) {
        errno = ENOMEM;
        return -1;
    }
    return write(ofd, padbuf, size);
}

int write_bitmap (int ofd, int leading_set_bits) {
    int bitmap_bytes_written;
    while (leading_set_bits > 8) { // Write whole set bytes.
        if (write(ofd, "\xFF", 1) != 1) {
            perror("write block bitmap");
            return 0;
        }
        leading_set_bits -= 8;
        bitmap_bytes_written++;
    }
    // This is not a mistake: the bitmaps count up from the LSB. Very dumb.
    char rembits = 0xFF >> (8 - (char)leading_set_bits);
    if (write(ofd, &rembits, 1) != 1) { // Write the remainder byte.
        perror("write block bitmap");
        return 0;
    }
    bitmap_bytes_written++;
    if (write_padding(ofd, EXT2_BLOCK_SIZE - bitmap_bytes_written) < 0) {
        perror("write padding");
        return 0;
    }
    return bitmap_bytes_written;
}

// TODO: Add more block groups.
int format_ext (int ofd) {
    struct ext2_group_desc groups[GROUPS_COUNT];
    memset(&groups, 0, sizeof(groups));
    int g = 0; // Index into groups.

    struct ext2_super_block sb;
    memset(&sb, 0, sizeof(sb));
    sb.s_magic = EXT2_SUPER_MAGIC;
    sb.s_log_block_size = 0; // Meaning block size is 1024 bytes.
    sb.s_log_cluster_size = 0 ; // Meaning fragment size is 1024 bytes.
    sb.s_block_group_nr = 1;
    sb.s_inodes_per_group = INODES_PER_GROUP;
    sb.s_blocks_per_group = BLOCKS_PER_GROUP;
    sb.s_mtime = 0; // Never mounted
    sb.s_wtime = 0; // Never written
    sb.s_state = EXT2_VALID_FS;
    sb.s_clusters_per_group = FRAGS_PER_GROUP;
    sb.s_first_data_block = S_FIRST_DATA_BLOCK;
    sb.s_mnt_count = 0;
    sb.s_max_mnt_count = 1000;
    sb.s_errors = EXT2_ERRORS_CONTINUE;
    sb.s_rev_level = EXT2_MAJOR_VERS;
    sb.s_minor_rev_level = EXT2_MINOR_VERS;
    sb.s_lastcheck = 0;
    sb.s_checkinterval = __UINT32_MAX__;
    sb.s_creator_os = EXT2_OS_LINUX;
    sb.s_def_resuid = ROOT_UID;
    sb.s_def_resgid = ROOT_GID;
    sb.s_inodes_count = INODES_COUNT;
    sb.s_r_blocks_count = 0;
    sb.s_blocks_count = sb.s_blocks_per_group * GROUPS_COUNT;

    struct ext2_group_desc gd;
    memset(&gd, 0, sizeof(gd));
    gd.bg_block_bitmap = 3; // Block bitmap will go right after.
    gd.bg_inode_bitmap = gd.bg_block_bitmap + BLOCKS_FOR_BLOCK_BITMAP;
    gd.bg_inode_table = gd.bg_inode_bitmap + BLOCKS_FOR_INODE_BITMAP;
    gd.bg_free_blocks_count = BLOCKS_PER_GROUP - (BLOCKS_FOR_BLOCK_BITMAP + BLOCKS_FOR_INODE_BITMAP + BLOCKS_FOR_INODE_TABLE);
    gd.bg_free_inodes_count = INODES_PER_GROUP - 1; // Just the root directory.
    gd.bg_used_dirs_count = 1; // 1 for root dir
    gd.bg_flags = 0;
    gd.bg_exclude_bitmap_lo = 0;
    gd.bg_block_bitmap_csum_lo = 0;
    gd.bg_inode_bitmap_csum_lo = 0;
    gd.bg_itable_unused = 0;
    gd.bg_checksum = 0;

    groups[g++] = gd;

    // These are defined as the sum of all block groups.
    for (int i = 0; i < g; i++) {
        sb.s_free_blocks_count = gd.bg_free_blocks_count;
        sb.s_free_inodes_count = gd.bg_free_inodes_count;
    }

    __u32 first_unused_data_block = gd.bg_inode_table + BLOCKS_FOR_INODE_TABLE;

    struct ext2_inode root_dir_inode;
    memset(&root_dir_inode, 0, sizeof(root_dir_inode));
    root_dir_inode.i_mode = LINUX_S_IFDIR | 0755;
    root_dir_inode.i_uid = ROOT_UID;
    root_dir_inode.i_size = 0; // The root directory is empty. No entries, no bytes.
    root_dir_inode.i_atime = 0; // Never accessed
    root_dir_inode.i_ctime = 1; // Created on Jan 1st, 1970. Why not?
    root_dir_inode.i_mtime = 0; // Never modified
    root_dir_inode.i_gid = ROOT_GID;
    root_dir_inode.i_links_count = 1; // I _think_ this has to be 1 to not get deleted.
    root_dir_inode.i_blocks = 1; // All of the root subdirs fit in a single block.
    root_dir_inode.i_flags = 0; 
    // root_dir_inode.i_osd1.linux1 = 0; How do I actually access this?
    root_dir_inode.i_block[0] = first_unused_data_block++;
    root_dir_inode.i_generation = 0;
    root_dir_inode.i_file_acl = 0;
    // There are more fields, but the documentation is not great, and I don't think they matter.

    if (lseek(ofd, SUPERBLOCK_OFFSET, SEEK_SET) < 0) {
        perror("lseek superblock");
        return EXIT_FAILURE;
    }
    if (write(ofd, &sb, sizeof(sb)) < 0) {
        perror("write superblock");
        return EXIT_FAILURE;
    }
    // Because superblock size == block size, we do not need to pad.

    if (write(ofd, &gd, sizeof(gd)) < 0) {
        perror("write block group descriptor");
        return EXIT_FAILURE;
    }
    if (write_padding(ofd, EXT2_BLOCK_SIZE - sizeof(gd)) < 0) {
        perror("write padding after block group descriptor table");
        return EXIT_FAILURE;
    }

    /* WRITE THE BLOCK BITMAP
        We have used blocks in order, so we can just count the number of blocks
        we used and set that many leading bits in the bitmap.

        leading bits = 
            1 for sb
            + 1 for gds
            + (gds count * (
                BLOCKS_FOR_BLOCK_BITMAP
                + BLOCKS_FOR_INODE_BITMAP
                + BLOCKS_FOR_INODE_TABLE))
            + 1 for root dir
    */
    int leading_bits_to_set = (
        1
        + 1
        + (g * (BLOCKS_FOR_BLOCK_BITMAP + BLOCKS_FOR_INODE_BITMAP + BLOCKS_FOR_INODE_TABLE))
        + 1
    );
    if (write_bitmap(ofd, leading_bits_to_set) <= 0) {
        perror("write block bitmap");
        return EXIT_FAILURE;
    }

    /* WRITE THE INODE BITMAP: The first 11 inodes are always reserved. */
    if (write(ofd, "\xFF", 1) != 1) {
        perror("write block bitmap");
        return EXIT_FAILURE;
    }
    if (write(ofd, "\x07", 1) != 1) {
        perror("write block bitmap");
        return EXIT_FAILURE;
    }
    if (write_padding(ofd, EXT2_BLOCK_SIZE - 1) < 0) {
        perror("write padding");
        return EXIT_FAILURE;
    }
    
    /* WRITE THE INODE TABLE */
    if (write_empty_inode(ofd) < 0) {
        perror("write empty inode");
        return EXIT_FAILURE;
    }
    if (write(ofd, &root_dir_inode, sizeof(root_dir_inode)) < 0) {
        perror("write root inode");
        return EXIT_FAILURE;
    }
    for (int i = 2; i < INODES_PER_GROUP; i++) {
        if (write_empty_inode(ofd) < 0) {
            perror("write empty inode");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main (int argc, char **argv) {
    int ofd = open(argv[1], O_CREAT | O_RDWR);
    if (ofd < 0) {
        perror("open file");
        return EXIT_FAILURE;
    }
    return format_ext(ofd);
}