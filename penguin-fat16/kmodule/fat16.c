#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include "fat16.h"
#include "super.h"

int fat16_next_cluster(struct super_block *sb, u16 *cluster)
{
	fat16_sb_info_t *sbi = FAT16_SBI(sb);
	u32 ent_per_sec = sbi->bytes_per_sector / sizeof(u16);
	u32 fat_sector  = sbi->fat_start + *cluster / ent_per_sec;
	u32 offset      = *cluster % ent_per_sec;
	struct buffer_head *bh;

	bh = sb_bread(sb, fat_sector);
	if (!bh)
		return -EIO;

	*cluster = le16_to_cpu(((u16 *)bh->b_data)[offset]);
	brelse(bh);
	return 0;
}

int fat16_read_cluster(struct super_block *sb, u32 cluster, void *buf)
{
	fat16_sb_info_t *sbi = FAT16_SBI(sb);
	u32 sector;
	u32 i;
	struct buffer_head *bh;

	if (cluster >= FAT16_FINAL_CLUSTER)
		return -EINVAL;

	sector = fat16_cluster_sector(sbi->data_start, cluster, sbi->sectors_per_cluster);

	for (i = 0; i < sbi->sectors_per_cluster; i++) {
		bh = sb_bread(sb, sector + i);
		if (!bh)
			return -EIO;
		memcpy(buf + (size_t)i * sbi->bytes_per_sector,
		       bh->b_data, sbi->bytes_per_sector);
		brelse(bh);
	}
	return 0;
}

int fat16_83_to_str(const u8 name83[FAT16_FILENAME_LEN], char *out, size_t outsz)
{
	size_t in_iter = 0;
	size_t out_iter = 0;

	while (in_iter < FAT16_STEM_LEN && name83[in_iter] != ' ') {
		if (out_iter >= outsz - 1)
			return -1;
		out[out_iter++] = name83[in_iter++];
	}

	if (name83[FAT16_STEM_LEN] != ' ') {
		if (out_iter >= outsz - 1)
			return -1;
		out[out_iter++] = '.';
		for (in_iter = FAT16_STEM_LEN;
		     in_iter < FAT16_FILENAME_LEN && name83[in_iter] != ' ';
		     in_iter++) {
			if (out_iter >= outsz - 1)
				return -1;
			out[out_iter++] = name83[in_iter];
		}
	}

	out[out_iter] = '\0';
	return 0;
}

int fat16_str_to_83(const char *name, size_t namelen, u8 out83[FAT16_FILENAME_LEN])
{
	char buf[FAT16_STEM_LEN + 1 + FAT16_EXTENSION_LEN + 1];

	if (namelen >= sizeof(buf))
		return -1;

	memcpy(buf, name, namelen);
	buf[namelen] = '\0';

	char *p = buf;
	const char *stem = strsep(&p, ".");
	const char *ext  = p ? p : "";

	size_t stem_len = strlen(stem);
	size_t ext_len  = strlen(ext);

	if (stem_len > FAT16_STEM_LEN || ext_len > FAT16_EXTENSION_LEN)
		return -1;

	memset(out83, ' ', FAT16_FILENAME_LEN);

	for (size_t i = 0; i < stem_len; ++i)
		out83[i] = toupper((unsigned char)stem[i]);

	for (size_t i = 0; i < ext_len; ++i)
		out83[FAT16_STEM_LEN + i] = toupper((unsigned char)ext[i]);

	return 0;
}
