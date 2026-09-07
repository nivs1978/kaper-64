// Centralized memory layout/constants for VIC/screen/charset/sprite setup.

.const VIC_BANK0_SELECT = $03
.const VIC_BANK1_SELECT = $02

.const VIC_D018_MAIN = $15      // screen $0400, charset $1000 (bank 0)
.const VIC_D018_INTRO = $12     // screen $4400, charset $4800 (bank 1)
.const INTRO_BITMAP_D018 = $78  // screen $5c00, bitmap $6000 (bank 1)
.const INTRO_BITMAP_SCREEN = $5c00
.const INTRO_BITMAP_END = $7f40    // one past the last unpacked byte
.const MAP_PACKED_ADDR = $a000     // packed map bitmap/screen data under the BASIC ROM
.const INTRO_PACKED_ADDR = $b340   // RLE-packed intro, under the BASIC ROM after MapArt

.const KERNAL_SCREEN_PAGE_ADDR = $0288
.const KERNAL_SCREEN_PAGE_MAIN = $04
.const KERNAL_SCREEN_PAGE_INTRO = $44

.const INTRO_SCREEN_RAM = $4400
.const MAP_BITMAP_ADDR = $6000
.const MAP_BITMAP_END = $7f40
.const MAP_SCREEN_END = INTRO_SCREEN_RAM + $03e8
// NOTE: charset must stay below $6000 - the map bitmap destination spans
// $6000-$7f3f and draw_map would overwrite anything placed inside it.
.const INTRO_CHARSET_RAM = $4800

.const SPRITE_PTR_TAB_BANK1_MAP = $47f8    // screen base $4400 + $03f8
.const SPRITE_PTR_TAB_BANK1_INTRO = $47f8  // screen base $4400 + $03f8

.const SPRITE_DATA_BANK1 = $5000

.const SPRITE_PTR_BANK1_BASE = 64

.const CHAR_SLOT_AE_ADDR = INTRO_CHARSET_RAM + $00d8
.const CHAR_SLOT_OE_ADDR = INTRO_CHARSET_RAM + $00e0
.const CHAR_SLOT_AA_ADDR = INTRO_CHARSET_RAM + $00e8
.const CHAR_SLOT_RIGHT_ARROW_ADDR = INTRO_CHARSET_RAM + $00f0
.const CHAR_SLOT_SPLASH_LT_ADDR = INTRO_CHARSET_RAM + ($3c * 8)
.const CHAR_SLOT_SPLASH_GT_ADDR = INTRO_CHARSET_RAM + ($3e * 8)
.const CHAR_SLOT_HARBOUR_SHIP_ADDR = INTRO_CHARSET_RAM + ($5e * 8)

// Shared zero-page pointers (declared here, first import, so every file that
// uses them for (zp),y indirect addressing gets true zero-page opcodes).
.label src_ptr = $fb
.label dst_ptr = $fd
.label len_lo = $f9
.label len_hi = $fa
.label bmp_char_ptr = $f7   // charset read pointer for draw_bmp_char (2 bytes: $f7=lo, $f8=hi)
.label inline_arg_ptr = $02 // scratch for the inline-argument print helpers ($02-$05 are unused by KERNAL)
.label inline_saved_x = $04
.label inline_saved_y = $05

// Print a zero-terminated string whose address is stored inline after the JSR.
// 5 bytes per call site instead of the 11-byte lda/sta src_ptr preamble.
.macro pstr(addr) {
        jsr print_str_inline
        .word addr
}
// Same, followed by a newline (replaces an extra 3-byte jsr print_newline).
.macro pstrnl(addr) {
        jsr print_str_inline_nl
        .word addr
}
// Print the 16-bit variable at `addr` in decimal, address stored inline after the JSR.
.macro pnum(addr) {
        jsr print_var_decimal
        .word addr
}
// Append a string to the screen buffer at the current set_screen_ptr_xy position.
.macro astr(addr) {
        jsr append_str_inline
        .word addr
}
// Write a string to the screen buffer at the current set_screen_ptr_xy position.
.macro wstr(addr) {
        jsr write_str_inline
        .word addr
}
// Draw a string into the map bitmap at bmp_char_col/bmp_char_row.
.macro bstr(addr) {
        jsr print_bmp_str_inline
        .word addr
}
// Draw the 16-bit variable at `addr` into the map bitmap stat field at X=col, Y=row.
.macro bnum(addr) {
        jsr print_bmp_stat_var
        .word addr
}
// Write the 16-bit variable at `addr` into the fight status field at X=col, Y=row.
.macro fnum(addr) {
        jsr print_fight_stat_var
        .word addr
}

.const MAP_GRID_COL_MIN = 1
.const MAP_GRID_COL_MAX = 29
.const MAP_GRID_ROW_MIN = 1
// Original BASIC clamps IY to 140 (rows 1..14 in 10-pixel steps).
.const MAP_GRID_ROW_MAX = 14
.const MAP_START_COL = 10
.const MAP_START_ROW = 10
.const MAP_GRID_STEP = 10
.const MAP_GRID_ORIGIN_X = 35
.const MAP_GRID_ORIGIN_Y = 59
.const MAP_START_PIXEL_X = 125
.const MAP_START_PIXEL_Y = 149
.const NAME_SCREEN_SHIP_X = 210
.const NAME_SCREEN_SHIP_Y = 72
.const MAP_PIXEL_MIN_X = MAP_GRID_ORIGIN_X + ((MAP_GRID_COL_MIN - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MAX_X = MAP_GRID_ORIGIN_X + ((MAP_GRID_COL_MAX - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MIN_Y = MAP_GRID_ORIGIN_Y + ((MAP_GRID_ROW_MIN - 1) * MAP_GRID_STEP)
.const MAP_PIXEL_MAX_Y = MAP_GRID_ORIGIN_Y + ((MAP_GRID_ROW_MAX - 1) * MAP_GRID_STEP)

.const MAP_CELL_SEA = 0
.const MAP_CELL_LAND = 1
// BASIC 3000/3080 write IMAP(IX/10,IY/10)=50 once an encounter there is resolved.
// The value matters only in that it matches nothing else the move handler tests,
// so the tile stays passable but never rolls another random event.
.const MAP_CELL_CLEARED = 50
.const MAP_HARBOUR_COUNT = 7
