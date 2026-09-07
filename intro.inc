.label intro_src_ptr = $fb
.label intro_dst_ptr = $fd

run_intro:
        // BASIC line 5: screen off while intro is prepared
        jsr hide_vic_screen

        lda #6
        sta $d020
        sta $d021

        lda #$93
        jsr $ffd2
        jsr configure_intro_vic_bank
        jsr prepare_intro_charset
        jsr decompress_intro_bitmap
        jsr draw_intro_press_key
        jsr audio_init
        jsr enable_intro_bitmap_mode
        lda #0
        sta $c6
        jsr show_vic_screen

        lda #<play_intro_theme
        ldx #>play_intro_theme
        jsr audio_play

intro_wait_release:
        jsr $ff9f
        jsr $ffe4
        bne intro_wait_release

intro_wait_key:
        jsr $ff9f
        jsr $ffe4
        beq intro_wait_key

intro_done:
        jsr audio_stop
        jsr disable_intro_bitmap_mode
        lda #14
        sta $d020
        lda #6
        sta $d021
        lda #0
        sta $c6
        jsr show_vic_screen
        rts

// Relocated out of the sub-$4400 code segment into the free gap after the ship
// sprite data; the routine is self-contained, so no branch crosses the boundary.
intro_code_end:
* = $8200 "RleUnpack"

// Shared RLE expander for the packed intro and map images.
// Caller sets src_ptr (packed data), dst_ptr (destination) and rle_end_lo/hi
// (one past the last byte to write).
// Token >= $80: run of (token & $7f) + 1 copies of the next byte.
// Token <  $80: (token + 1) literal bytes follow.
rle_unpack:
        ldy #0
        lda (src_ptr),y
        jsr rle_next_src
        tax
        bmi rle_run
        inx                         // literal count = token + 1
rle_literal:
        ldy #0
        lda (src_ptr),y
        sta (dst_ptr),y
        jsr rle_next_src
        jsr rle_next_dst
        dex
        bne rle_literal
        beq rle_check_end           // always: dex left X at 0

rle_run:
        txa
        and #$7f
        tax
        inx                         // run count = (token & $7f) + 1
        ldy #0
        lda (src_ptr),y             // repeated value, kept in A across the loop
        jsr rle_next_src
rle_run_store:
        ldy #0
        sta (dst_ptr),y
        jsr rle_next_dst
        dex
        bne rle_run_store

rle_check_end:
        lda dst_ptr+1
        cmp rle_end_hi
        bne rle_unpack
        lda dst_ptr
        cmp rle_end_lo
        bne rle_unpack
        rts

// Both must leave A untouched: the run loop keeps its fill byte there.
rle_next_src:
        inc src_ptr
        bne rle_next_src_done
        inc src_ptr+1
rle_next_src_done:
        rts

rle_next_dst:
        inc dst_ptr
        bne rle_next_dst_done
        inc dst_ptr+1
rle_next_dst_done:
        rts

rle_end_lo: .byte 0
rle_end_hi: .byte 0

// The packed intro sits under the BASIC ROM, so bank it out while reading.
decompress_intro_bitmap:
        sei
        lda $01
        pha
        and #$fe
        sta $01
        lda #<intro_rle_data
        sta src_ptr
        lda #>intro_rle_data
        sta src_ptr+1
        lda #<INTRO_BITMAP_SCREEN
        sta dst_ptr
        lda #>INTRO_BITMAP_SCREEN
        sta dst_ptr+1
        lda #<INTRO_BITMAP_END
        sta rle_end_lo
        lda #>INTRO_BITMAP_END
        sta rle_end_hi
        jsr rle_unpack
        pla
        sta $01
        cli
        rts

decompress_map_bitmap:
        sei
        lda $01
        pha
        and #$fe
        sta $01
        lda #<map_bitmap_rle_data
        sta src_ptr
        lda #>map_bitmap_rle_data
        sta src_ptr+1
        lda #<MAP_BITMAP_ADDR
        sta dst_ptr
        lda #>MAP_BITMAP_ADDR
        sta dst_ptr+1
        lda #<MAP_BITMAP_END
        sta rle_end_lo
        lda #>MAP_BITMAP_END
        sta rle_end_hi
        jsr rle_unpack
        pla
        sta $01
        cli
        rts

decompress_map_screen:
        sei
        lda $01
        pha
        and #$fe
        sta $01
        lda #<map_screen_rle_data
        sta src_ptr
        lda #>map_screen_rle_data
        sta src_ptr+1
        lda #<INTRO_SCREEN_RAM
        sta dst_ptr
        lda #>INTRO_SCREEN_RAM
        sta dst_ptr+1
        lda #<MAP_SCREEN_END
        sta rle_end_lo
        lda #>MAP_SCREEN_END
        sta rle_end_hi
        jsr rle_unpack
        pla
        sta $01
        cli
        rts

* = intro_code_end

// Draws the prompt into the intro hires bitmap's bottom-left character row.
// draw_bmp_char writes its colour byte to the $4400 screen, so the colour cells
// for the intro's own screen at $5c00 have to be filled separately.
draw_intro_press_key:
        lda #0
        sta bmp_char_col
        lda #24
        sta bmp_char_row
        bstr(intro_press_key)
        ldx #0
ipk_colour:
        lda #$16                    // white fg on dark blue bg
        sta INTRO_BITMAP_SCREEN + 24 * 40, x
        inx
        cpx #intro_press_key_end - intro_press_key - 1
        bne ipk_colour
        rts

intro_press_key:
        .text "TRYK P] EN TAST FOR AT FORTS[TTE!"
        .byte 0
intro_press_key_end:

enable_intro_bitmap_mode:
        lda #INTRO_BITMAP_D018
        sta $d018
        lda $d016
        and #$ef                    // hires, not multicolor
        sta $d016
        lda $d011
        ora #$20                    // BMM on
        sta $d011
        rts

disable_intro_bitmap_mode:
        jsr hide_vic_screen
        lda $d011
        and #$df
        sta $d011
        lda #VIC_D018_INTRO
        sta $d018
        rts

// ---------------------------------------------------------------------------
// Gameplay SID sounds. Voice 1 is reserved for these short, blocking effects;
// the intro music is stopped before gameplay begins.

sound_toggle:
        lda sound_enabled
        eor #1
        sta sound_enabled
        beq sound_toggle_off
        lda #<play_toggle_beep
        ldx #>play_toggle_beep
        jmp audio_play
sound_toggle_off:
        jmp audio_stop

sound_grounded:
        lda #<play_grounded
        ldx #>play_grounded
        jmp audio_play

sound_harbour_collision:
        lda #<play_harbour_collision
        ldx #>play_harbour_collision
        jmp audio_play

sound_flee:
        lda #<play_flee
        ldx #>play_flee
        jmp audio_play

sound_cannon:
        lda #<play_cannon
        ldx #>play_cannon
        jmp audio_play

sound_splash:
        lda #<play_splash
        ldx #>play_splash
        jmp audio_play

sound_fog:
        lda #<play_fog_bank_warning
        ldx #>play_fog_bank_warning
        jmp audio_play

sound_surrender:
        lda #<play_boarding_victory
        ldx #>play_boarding_victory
        jmp audio_play

sound_sink:
        lda #<play_ship_sinking
        ldx #>play_ship_sinking
        jmp audio_play

sound_boarding:
        lda #<play_boarding_fight
        ldx #>play_boarding_fight
        jmp audio_play

sound_promotion:
        lda #<play_promotion
        ldx #>play_promotion
        jmp audio_play

hide_vic_screen:
        lda $d011
        and #$ef
        sta $d011
        rts

show_vic_screen:
        lda $d011
        ora #$10
        sta $d011
        rts

configure_intro_vic_bank:
        // VIC bank 1 ($4000-$7fff), screen $4400, charset $4800.
        jsr hide_vic_screen
        lda $dd00
        and #$fc
        ora #VIC_BANK1_SELECT
        sta $dd00
        lda #VIC_D018_INTRO
        sta $d018
        // KERNAL screen editor should also target $4400 (high byte $44).
        lda #KERNAL_SCREEN_PAGE_INTRO
        sta KERNAL_SCREEN_PAGE_ADDR
        rts

restore_main_vic_bank:
        // Restore default VIC bank 0 and screen/charset pointer.
        lda $dd00
        and #$fc
        ora #VIC_BANK0_SELECT
        sta $dd00
        lda #VIC_D018_MAIN
        sta $d018
        lda #KERNAL_SCREEN_PAGE_MAIN
        sta KERNAL_SCREEN_PAGE_ADDR
        rts

prepare_intro_charset:
        // Copy uppercase ROM charset ($D000-$D7FF) to RAM at $4800.
        sei
        lda $01
        pha
        and #$fb
        sta $01

        lda #$00
        sta intro_src_ptr
        sta intro_dst_ptr
        lda #$d0
        sta intro_src_ptr+1
        lda #>INTRO_CHARSET_RAM
        sta intro_dst_ptr+1

        ldx #8
        ldy #0
charset_copy_page:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        bne charset_copy_page
        inc intro_src_ptr+1
        inc intro_dst_ptr+1
        dex
        bne charset_copy_page

        pla
        sta $01
        cli

        // Patch custom Danish glyphs: [=AE, \=OE, ]=AA (Danish AE/OE/AA; screen codes 27/28/29).
        // Also patch ^=right arrow (screen code 30); _ keeps its default left-arrow glyph.
        ldy #0
charset_patch_ae:
        lda char_ae,y
        sta CHAR_SLOT_AE_ADDR,y
        iny
        cpy #8
        bne charset_patch_ae

        ldy #0
charset_patch_oe:
        lda char_oe,y
        sta CHAR_SLOT_OE_ADDR,y
        iny
        cpy #8
        bne charset_patch_oe

        ldy #0
charset_patch_aa:
        lda char_aa,y
        sta CHAR_SLOT_AA_ADDR,y
        iny
        cpy #8
        bne charset_patch_aa

        ldy #0
charset_patch_right_arrow:
        lda char_right_arrow,y
        sta CHAR_SLOT_RIGHT_ARROW_ADDR,y
        iny
        cpy #8
        bne charset_patch_right_arrow

        ldy #0
charset_patch_splash_lt:
        lda char_splash_lt,y
        sta CHAR_SLOT_SPLASH_LT_ADDR,y
        iny
        cpy #8
        bne charset_patch_splash_lt

        ldy #0
charset_patch_splash_gt:
        lda char_splash_gt,y
        sta CHAR_SLOT_SPLASH_GT_ADDR,y
        iny
        cpy #8
        bne charset_patch_splash_gt

        ldy #0
charset_patch_harbour_ship:
        lda char_harbour_ship,y
        sta CHAR_SLOT_HARBOUR_SHIP_ADDR,y
        iny
        cpy #8
        bne charset_patch_harbour_ship
        rts

copy_1000_bytes:
        ldy #0
        ldx #3
copy_full_page:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        bne copy_full_page
        inc intro_src_ptr+1
        inc intro_dst_ptr+1
        dex
        bne copy_full_page

        ldy #0
copy_tail_232:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        iny
        cpy #232
        bne copy_tail_232
        rts

// Fallback labels so intro code can stay assembled while intro_screen include is disabled.
//.label intro_screen_data = 0
//.label intro_color_data  = 0

// glyph data from MAIN.BAS lines 122-128
char_ae:     .byte 63,108,108,127,108,108,111,0
char_oe:     .byte 60,102,110,126,118,102,60,0
char_aa:     .byte 24,36,24,60,102,126,102,0
char_right_arrow: .byte 0,4,6,127,127,6,4,0
char_splash_lt: .byte 0,0,0,24,126,165,126,24
char_splash_gt: .byte 60,90,24,153,126,165,126,24
char_harbour_ship: .byte 36,36,36,36,36,255,126,126
