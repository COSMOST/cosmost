#ifndef __STUDY_FILEDB_HELPER_H__
#define __STUDY_FILEDB_HELPER_H__

static inline
void string16_to_string8(const char *src_buf, char *dst_buf, uint32_t len)
{
	uint32_t i, j;
	for (i = 0, j = 0; j < len; i += sizeof(char16_t), j++)
		dst_buf[j] = src_buf[i];
}

#endif /* __STUDY_FILEDB_HELPER_H__ */
