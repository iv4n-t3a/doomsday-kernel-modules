#ifndef PENGUIN_FAT16_FAT16_H
#define PENGUIN_FAT16_FAT16_H

#include <linux/fs.h>
#include <linux/types.h>

#define FAT16_STEM_LEN      8
#define FAT16_EXTENSION_LEN 3
#define FAT16_FILENAME_LEN  11
#define FAT16_FINAL_CLUSTER 0xFFF8u

#define FAT16_ATTR_READ_ONLY  0x01
#define FAT16_ATTR_HIDDEN     0x02
#define FAT16_ATTR_SYSTEM     0x04
#define FAT16_ATTR_VOLUME_ID  0x08
#define FAT16_ATTR_DIRECTORY  0x10
#define FAT16_ATTR_ARCHIVE    0x20
#define FAT16_ATTR_LFN        0x0F

#define FAT16_DELETED_MAGIC   0xE5

static const union { char s[8]; u64 v; } _fat16_fs_magic_u = { .s = "FAT16" };
#define fat16_fs_magic (_fat16_fs_magic_u.v)

typedef struct {
	u8  bootjmp[3];
	u8  OEM_identifier[8];
	u16 bytes_per_sector;
	u8  sectors_per_cluster;
	u16 reserved_sectors;
	u8  FAT_count;
	u16 root_dir_entries;
	u16 sectors_per_volume_u16;
	u8  media_descriptor_type;
	u16 sectors_per_FAT;
	u16 sectors_per_track;
	u16 number_of_heads;
	u32 part_begin_lba;
	u32 sectors_per_volume_u32;
	u8  drive_number;
	u8  windows_nt_flags;
	u8  signature;
	u32 volume_id;
	char volume_label[11];
	char system_identifier[8];
} __attribute__((packed)) fat16_bpb_t;

typedef struct {
	u8  filename[FAT16_FILENAME_LEN];
	u8  attr;
	u8  ntres;
	u8  creation_time_tenth;
	u16 creation_time;
	u16 creation_date;
	u16 last_access_date;
	u16 first_cluster_high;
	u16 write_time;
	u16 write_date;
	u16 first_cluster_low;
	u32 file_size;
} __attribute__((packed)) fat16_direntry_t;

static inline u32 fat16_fat_sector(const fat16_bpb_t *bpb)
{
	return bpb->reserved_sectors;
}

static inline u32 fat16_root_dir_sector(const fat16_bpb_t *bpb)
{
	return fat16_fat_sector(bpb) + (u32)bpb->FAT_count * bpb->sectors_per_FAT;
}

static inline u32 fat16_bytes_per_cluster(const fat16_bpb_t *bpb)
{
	return (u32)bpb->bytes_per_sector * bpb->sectors_per_cluster;
}

static inline u32 fat16_entries_per_sector(u32 bytes_per_sector)
{
	return bytes_per_sector / sizeof(fat16_direntry_t);
}

static inline u32 fat16_entries_per_cluster(u32 bytes_per_cluster)
{
	return bytes_per_cluster / sizeof(fat16_direntry_t);
}

static inline u32 fat16_root_sectors(u32 root_dir_entries, u32 bytes_per_sector)
{
	return (root_dir_entries * sizeof(fat16_direntry_t) + bytes_per_sector - 1)
	       / bytes_per_sector;
}

static inline u32 fat16_cluster_sector(u32 data_start, u32 cluster, u32 sectors_per_cluster)
{
	return data_start + (cluster - 2) * sectors_per_cluster;
}

static inline u32 fat16_entry_sector(u32 cluster_sector, u32 i, u32 entries_per_sector)
{
	return cluster_sector + i / entries_per_sector;
}

static inline u32 fat16_entry_idx(u32 i, u32 entries_per_sector)
{
	return i % entries_per_sector;
}

static inline u32 fat16_root_dir_sectors(const fat16_bpb_t *bpb)
{
	return fat16_root_sectors(bpb->root_dir_entries, bpb->bytes_per_sector);
}

static inline u32 fat16_data_sector(const fat16_bpb_t *bpb)
{
	return fat16_root_dir_sector(bpb) + fat16_root_dir_sectors(bpb);
}

static inline bool fat16_de_is_end(const fat16_direntry_t *de)
{
	return de->filename[0] == 0x00;
}

static inline bool fat16_de_is_skip(const fat16_direntry_t *de)
{
	return de->filename[0] == FAT16_DELETED_MAGIC ||
	       de->attr == FAT16_ATTR_LFN ||
	       (de->attr & FAT16_ATTR_VOLUME_ID);
}

static inline bool fat16_de_is_dir(const fat16_direntry_t *de)
{
	return de->attr & FAT16_ATTR_DIRECTORY;
}

int fat16_next_cluster(struct super_block *sb, u16 *cluster);
int fat16_read_cluster(struct super_block *sb, u32 cluster, void *buf);
int fat16_83_to_str(const u8 name83[FAT16_FILENAME_LEN], char *out, size_t outsz);
int fat16_str_to_83(const char *name, size_t namelen, u8 out83[FAT16_FILENAME_LEN]);

#endif  // #ifndef PENGUIN_FAT16_FAT16_H
