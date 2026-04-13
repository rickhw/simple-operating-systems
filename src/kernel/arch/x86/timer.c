#include <stddef.h>
#include "timer.h"
#include "io.h"
#include "utils.h"
#include "task.h"
#include "gui.h"

// 計時器晶片 8254 Programmable Interval Timer (PIT)
// 負責產生的規律電子訊號，他的基礎頻率是 1.19318 MHz (1,193,180 Hz)，
// 頻率則透過石英振蕩器生成 (Crystal Oscillator)
//
//     All PC compatibles operate the PIT at a clock rate of 105/88 = 1.19318 MHz
//
// 1,193,180 這個數字緣由是彩色電視 NTSC 的副載波頻率（14.31818 MHz）經過 12 分頻計算得來。
// 在電子工程中，「分頻 (Frequency Division)」指的是將一個高頻率的訊號，
// 轉換成較低頻率訊號的過程。簡單說「12 分頻」就是把原始頻率除以 12。
//
//     14.31818 MHz / 12 = 1,193,181 Hz
//
// 作業系統約定成俗的會使用 1,193,180 或 1,193,182 當常數
//
// 80 年代（IBM PC 誕生初期），石英震盪器（Oscillator）是很貴的零件。
// 工程師為了省錢，不想在主機板上插滿各種不同頻率的石英鐘，於是採用了「一魚多吃」
//
// - 主頻源：只買一個最便宜、大量生產的 14.31818 MHz 石英震盪器（當時因為彩色電視普及，這種零件極度便宜）。
// - 給顯卡用：這個頻率直接給彩色顯示卡使用，因為它剛好是 NTSC 電視訊號的 4 倍。
// - 給 CPU 用：經過 3 分頻 (14.318 / 3 = 4.77 MHz)，這就是初代 IBM PC 處理器的時脈。
// - 給計時器用：經過 12 分頻 (14.318 / 12 = 1.19 MHz)，這就是 8254 PIT 晶片拿到的工作頻率。
//
// @see: INTEL 8253/8254: https://en.wikipedia.org/wiki/Intel_8253
// @see: https://en.wikipedia.org/wiki/Crystal_oscillator
#define PIT_CLOCK_RATE 1193180

// tick (狀聲詞, 滴答聲)，用來記錄 作業系統 的節拍數，從開機就開始計算。
//
// 32bit 整數，在 100Hz 下頻率約 497 天會溢位。
//
// volatile 是告訴編譯器：「這個變數的值可能會在程式流程之外（被硬體中斷）改變」。
// 如果不加 volatile，編譯器可能會為了優化而把 tick 的值存在暫存器裡，導致其他程式碼讀到舊的值。
volatile uint32_t tick = 0;

// @param frequency: 每秒觸發中斷的次數
// 如果設定為 100Hz，每個 tick 的時間長就是 10ms；若改為 1000Hz，則可以精確到 1ms。
//
// 在 main.c 裡面的初始流程設定為 100
void init_timer(uint32_t frequency) {

    // divisor 表示晶片數 N 次，就發出一次中斷。
    //
    // 用 frequency 當基數，如果想要每秒中斷 100 次，frequency=100，
    // 則晶片每數 (PIT8254_BASE_FREQ / frequency = 11931 次訊號，
    // 就發出一次中斷。
    uint32_t divisor = PIT_CLOCK_RATE / frequency;

    // x43: PIT 的 模式控制暫存器 埠號。
    // 0x36 (00110110b): 這是控制字組（Control Word）。
    //  - 00: 選擇通道 0（接在 IRQ0，負責系統時鐘）。
    //  - 11: 存取模式 (LOBYTE/HIBYTE)，先寫低八位元，再寫高八位元
    //  - 011: 選擇模式 3（方波產生器，最適合當作時鐘）。
    //         它會自動重新加載計數器，適合產生週期性心跳。
    outb(0x43, 0x36);

    // 取得 divisor 的低 8 位元 (low byte) 和 高八位元 (high byte)
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    // 把 divisor 塞到 8254 暫存器, 但 8254 是 8bit 匯流排
    // 所以分成兩次, 把高低位原 依序「餵」進去
    //
    // 0x40: 這是通道 0 的 資料暫存器
    outb(0x40, l);
    outb(0x40, h);
}

// 中斷發生後的處理流程，由組合語言 interrupts.S#isr32 調用
//
// [Notes] 這段類似於 Game Loop 中的 Update(),
// 差別在於 GameLoop 是由 Game Engine 控制 (或我自己寫的 `while(true) {}`)
// OS 則由 PIT 晶片控制，透過中斷的方式製造出 callback 效果。
//
// @see: Game Loop https://github.com/rickhw/java-2d-rpg-game/blob/main/src/main/java/gtcafe/rpg/GamePanel.java#L217
// @see: Update() https://github.com/rickhw/java-2d-rpg-game/blob/main/src/main/java/gtcafe/rpg/GamePanel.java#L274
void timer_handler(void) {
    // 這裡的 tick++ 就像是 OS 的「虛擬時間」步進。
    // 在 x86 指令中，這是一個原子操作或受中斷保護的操作，
    // 確保所有依賴時間的 Syscall (如 sleep) 都有統一的參考系。
    //
    // [Notes]
    // 定義作業系統的節奏感，每次的累加，都代表著作業系統時間的流逝
    //
    // 概念類似於 Game Loop 裡的 FPS。但不同的是，
    // Game Loop 會因為 Update 處理的效能 (通常是 2D/3D 圖形渲染)
    // 因而影響 FPS 的大小。
    //
    // 作業系統則不能因為 task 處理慢了，tick 就停下來。
    tick++;

    // CPU Accounting (資源統計)
    // 計算 CPU 使用率核心邏輯，現在是誰在用 CPU，
    // 這個滴答的功勞就記在誰頭上！
    //
    // 作業系統無法無時無刻監控 CPU 資源。
    // 採用「抽樣法」——當中斷發生時，誰在位置上，
    // 這過去的 10ms 算費就記在誰頭上。
    //
    // 這是 top 指令顯示 CPU 佔用率的數據來源。
    if (current_task != 0) {
        current_task->total_ticks++;
    }

    // 每一滴答 (Tick)，檢查檢查滑鼠有沒有動、視窗是否需要重繪。
    // 這保證了 UI 的反應速度與 Tick 頻率同步。
    //
    // [Notes]
    // 這段呈現出經典的 Windows 95 搖動滑鼠會改善效率的問題
    // @see: https://www.techbang.com/posts/71423-this-theory-suggests-wiggling-the-mouse-cursor-actually-did-make-windows-95-run-faster
    gui_handle_timer();

    // @term: Programmable Interrupt Control (PIC), 中斷控制器, 通常是 INTEL 8259 這顆晶片
    // @term: End of Interrupt (EOI)
    // @purpose: OS 告訴 PIC
    //
    //     我處理完了，可以發下一個中斷給我了。
    //
    // PIC 介於所有硬體與 CPU 之間的「通訊官」，他像是一個警衛，他發出中斷後會擋住後續的所有訊號。
    // 如果 OS 不回覆 EOI，警衛會以為你還在忙，從此以後再也不會發出任何中斷訊號給 CPU，
    // 系統就會看起來像是「當機」了。
    //
    // 因為 schedule() 會導致上下文切換 (Context Switch)，
    // 如果不送 EOI 指令，CPU 就會被鎖死，再也不會收到下一個 tick。
    //
    // @param: 0x20 (埠號): 這是 Master PIC (中斷控制器) 的命令暫存器。
    // @value: 0x20 (數值): 這是 EOI (End of Interrupt) 指令。
    //
    // @see: INTEL 8259 https://en.wikipedia.org/wiki/Intel_8259
    outb(0x20, 0x20);

    // 實踐 搶佔式多工 (Preemptive Multitasking)，強行切換任務
    // 強制在 timer_handler 結束前切換到另一個任務，
    // 達成「每個程式都在同時執行」的假象。
    schedule();
}
