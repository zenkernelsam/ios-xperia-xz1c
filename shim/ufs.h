#pragma once
int test_ufs(void);

uint32_t ufs_write_blocks(uint32_t lba, uint32_t count, const void *buf);
uint32_t ufs_read_blocks(uint32_t lba, uint32_t count, void *buf);
uint32_t ufs_sync_cache(void);
uint32_t ufs_hibernate(void);
