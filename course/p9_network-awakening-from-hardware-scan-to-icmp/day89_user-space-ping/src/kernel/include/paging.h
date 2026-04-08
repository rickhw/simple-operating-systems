#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void init_paging(void);

// 將特定的實體位址 (phys) 映射到虛擬位址 (virt)
// flags 是權限標籤 (例如：3 代表 Present + Read/Write)
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

// 專門用來映射 Framebuffer (MMIO, Memory-Mapped I/O) 的安全函數
void map_vram(uint32_t virt, uint32_t phys);

uint32_t paging_get_active_universes(void);

uint32_t create_page_directory(void);
void free_page_directory(uint32_t pd_phys);

#endif
