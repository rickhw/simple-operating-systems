#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/**
 * @brief 初始化分頁 (Paging) 系統，建立 Kernel 基礎的記憶體映射。
 */
void init_paging(void);

/**
 * @brief 將實體位址 (Physical) 映射到虛擬位址 (Virtual)。
 * * @param virt  欲映射的虛擬位址。
 * @param phys  對應的實體位址。
 * @param flags 權限標籤。常見組合：
 * - 3 (0b011): Present + Read/Write (Kernel 模式)
 * - 7 (0b111): Present + Read/Write + User (User 模式)
 */
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/**
 * @brief 專門用來映射 Video RAM / Framebuffer (MMIO) 的輔助函數。
 * * @param virt 虛擬位址。
 * @param phys 實體位址。
 */
void map_vram(uint32_t virt, uint32_t phys);

/**
 * @brief 取得目前已分配的 Universe (獨立位址空間) 總數。
 * * @return 正在使用的 Universe 數量。
 */
uint32_t paging_get_active_universes(void);

/**
 * @brief 為新的 Process 建立一個獨立的 Page Directory (Universe)。
 * * @return 新 Page Directory 的實體位址。
 */
uint32_t create_page_directory(void);

/**
 * @brief 釋放指定的 Page Directory，並回收其所佔用的實體記憶體。
 * * @param pd_phys 欲釋放的 Page Directory 實體位址。
 */
void free_page_directory(uint32_t pd_phys);

#endif
