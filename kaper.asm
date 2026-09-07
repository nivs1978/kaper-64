        // Kaper 64 - Kaptajn Kaper i Kattegat
        // Original Author: P.O. Frederiksen
        // Commodore 64 port by Hans Milling
        // License: GNU General Public License v3.0; see LICENSE in the repository.

        BasicUpstart2(start)
        .encoding "petscii_upper"
                .import source "constants.inc"

        * = $0810
start:
        // Intro installs a RAM charset at $1000 after screen/color data has been copied.

        lda #0
        sta skip_title_screen
        jmp start_setup

// Entered after a game over: the intro is unpacked fresh each time it runs, so
// only the title screen needs skipping.
restart_game:
        lda #1
        sta skip_title_screen

start_setup:
        jsr load_ship_sprites
        lda #<MAP_START_PIXEL_X
        sta ship_x
        lda #>MAP_START_PIXEL_X
        sta ship_x_hi
        lda #MAP_START_PIXEL_Y
        sta ship_y

        lda $dd00
        and #$fc
        ora #$03
        sta $dd00

        lda $d011
        and #$9f        // clear ECM+BMM; preserve DEN
        sta $d011

        lda $d016
        and #$ef
        sta $d016

        lda #1
        sta sound_enabled

        // clear screen then print each custom char on its own line
        lda #$93
        jsr $ffd2

        lda skip_title_screen
        bne start_skip_title
        jsr show_start_screen
start_skip_title:
        jsr run_intro
        jmp done

done:
        // ask for name - MAIN.BAS lines 2010-2160
        lda #$93
        jsr $ffd2
        lda #$05        // white text
        jsr $ffd2
        jsr print_double_newline

        pstr(askstr)
        jsr print_double_newline

        // Build-time diagnostic: bytes still free before code would reach into
        // $4400 (VIC bank 1 screen RAM), so an overlap regression is easy to spot.
        pstr(freememstr)
        lda #<($4400 - code_segment_end)
        sta num_print_lo
        lda #>($4400 - code_segment_end)
        sta num_print_hi
        jsr print_inline_word_decimal
        jsr print_double_newline

        // CHROUT clear can overwrite pointer bytes in the active screen's pointer table.
        jsr set_sprite_pointers_intro

        lda #<NAME_SCREEN_SHIP_X
        sta ship_x
        lda #>NAME_SCREEN_SHIP_X
        sta ship_x_hi
        lda #NAME_SCREEN_SHIP_Y
        sta ship_y
        jsr position_ship
        jsr show_ship

        pstr(hvadstr)

        lda #0
        sta $cc         // enable cursor blink
        ldx #0
inloop: jsr $ffe4
        beq inloop
        cmp #13
        beq indone
        cmp #20         // DEL
        beq indel
        cpx #15
        bcs inloop
        sta namebuf,x
        inx
        jsr $ffd2
        jmp inloop
indel:  cpx #0
        beq inloop
        dex
        lda #20
        jsr $ffd2
        jmp inloop
indone: lda #0
        sta namebuf,x
        lda #$ff
        sta $cc         // disable cursor blink
        cpx #0
        bne nameok
        ldx #0          // default to "KAPER"
defloop:lda kaperstr,x
        sta namebuf,x
        beq nameok
        inx
        bne defloop
nameok: lda #13
        jsr $ffd2
        jsr draw_map
        jsr load_ship_sprites
        jsr init_map_navigation
        jsr show_ship
        jsr game_loop
        rts

load_ship_sprites:
        jsr set_sprite_pointers_map
        jsr set_sprite_pointers_intro

        lda #$0a
        sta $d025
        lda #$0f
        sta $d026

        lda #$02
        sta $d027
        lda #$0b
        sta $d028
        lda #$05
        sta $d029

        // Force a known sprite state: only sprites 0-2 are used by the ship.
        lda #$00
        sta $d010
        sta $d01c
        sta $d017
        sta $d01d
        sta $d01b

        ldy #0
shipcopy:
        lda ship_sprites,y
        sta SPRITE_DATA_BANK1,y
        iny
        cpy #192
        bne shipcopy
        rts

// Boarding scene: both ships sit in the empty top-right block (rows 0-5), above
// the fight text which starts at row 6. Sprite X 24 = column 0, Y 50 = row 0.
.const BOARD_SHIP_A_X = 240         // stationary ship, columns 27-32
.const BOARD_SHIP_A_Y = 50
.const BOARD_SHIP_B_START_X = BOARD_SHIP_A_X + 50
.const BOARD_SHIP_B_START_Y = BOARD_SHIP_A_Y + 50
.const BOARD_SHIP_B_TARGET_X = BOARD_SHIP_A_X + 8
.const BOARD_SHIP_B_TARGET_Y = BOARD_SHIP_A_Y + 8

boarding_ship_a_x:          .word BOARD_SHIP_A_X
boarding_ship_a_y:          .byte BOARD_SHIP_A_Y
boarding_ship_b_x:          .word BOARD_SHIP_B_START_X
boarding_ship_b_y:          .byte BOARD_SHIP_B_START_Y
boarding_ship_step:         .byte 2
boarding_x:                 .word 0
boarding_y:                 .byte 0
boarding_base_slot:         .byte 0
boarding_sprite_bits:       .byte $01,$02,$04,$08,$10,$20,$40,$80

load_boarding_ship_sprites:
        lda #$04
        sta $d025
        lda #$03
        sta $d026

        lda #$0f
        sta $d027
        sta $d028
        sta $d029
        sta $d02a
        sta $d02b
        sta $d02c
        sta $d02d
        sta $d02e

        lda #$00
        sta $d010
        sta $d017
        sta $d01d
        sta $d01b
        lda #$ff
        sta $d01c

        // Both ships share the same four 24x21 tiles, so sprites 0-3 and 4-7
        // point at blocks 64-67 ($5000-$50ff). Using eight distinct blocks
        // would spill into $5100, which holds the shooting crosshair sprite.
        ldx #0
board_ptr_setup:
        txa
        and #$03
        clc
        adc #64
        sta SPRITE_PTR_TAB_BANK1_MAP,x
        inx
        cpx #8
        bne board_ptr_setup

        ldx #0
board_ship_copy_1:
        lda large_boarding_ship_1,x
        sta SPRITE_DATA_BANK1,x
        inx
        cpx #64
        bne board_ship_copy_1

        ldx #0
board_ship_copy_2:
        lda large_boarding_ship_2,x
        sta SPRITE_DATA_BANK1 + 64,x
        inx
        cpx #64
        bne board_ship_copy_2

        ldx #0
board_ship_copy_3:
        lda large_boarding_ship_3,x
        sta SPRITE_DATA_BANK1 + 128,x
        inx
        cpx #64
        bne board_ship_copy_3

        ldx #0
board_ship_copy_4:
        lda large_boarding_ship_4,x
        sta SPRITE_DATA_BANK1 + 192,x
        inx
        cpx #64
        bne board_ship_copy_4

        lda #$ff
        sta $d015
        rts

// A = sprite index 0-7, boarding_x/boarding_y = position (X is 16-bit).
set_boarding_sprite:
        tay
        asl
        tax
        lda boarding_x
        sta $d000,x
        lda boarding_y
        sta $d001,x
        lda boarding_sprite_bits,y
        ldy boarding_x+1
        bne board_msb_set
        eor #$ff
        and $d010
        sta $d010
        rts
board_msb_set:
        ora $d010
        sta $d010
        rts

// A = first sprite index (0 or 4); places a 2x2 ship at boarding_x/boarding_y.
place_boarding_ship:
        sta boarding_base_slot
        jsr set_boarding_sprite
        lda boarding_x
        clc
        adc #24
        sta boarding_x
        bcc board_place_tr
        inc boarding_x+1
board_place_tr:
        lda boarding_base_slot
        clc
        adc #1
        jsr set_boarding_sprite
        lda boarding_y
        clc
        adc #21
        sta boarding_y
        lda boarding_base_slot
        clc
        adc #3
        jsr set_boarding_sprite
        lda boarding_x
        sec
        sbc #24
        sta boarding_x
        bcs board_place_bl
        dec boarding_x+1
board_place_bl:
        lda boarding_base_slot
        clc
        adc #2
        jsr set_boarding_sprite
        rts

set_boarding_ship_positions:
        lda boarding_ship_a_x
        sta boarding_x
        lda boarding_ship_a_x+1
        sta boarding_x+1
        lda boarding_ship_a_y
        sta boarding_y
        lda #0
        jsr place_boarding_ship

        lda boarding_ship_b_x
        sta boarding_x
        lda boarding_ship_b_x+1
        sta boarding_x+1
        lda boarding_ship_b_y
        sta boarding_y
        lda #4
        jsr place_boarding_ship
        rts

// Ship B closes in diagonally (left and up) until it overlaps ship A.
run_boarding_ship_animation:
        jsr load_boarding_ship_sprites
        lda #<BOARD_SHIP_B_START_X
        sta boarding_ship_b_x
        lda #>BOARD_SHIP_B_START_X
        sta boarding_ship_b_x+1
        lda #BOARD_SHIP_B_START_Y
        sta boarding_ship_b_y
        jsr set_boarding_ship_positions
board_anim_loop:
        lda boarding_ship_b_x
        cmp #<BOARD_SHIP_B_TARGET_X
        lda boarding_ship_b_x+1
        sbc #>BOARD_SHIP_B_TARGET_X
        bcc board_anim_done
        lda boarding_ship_b_x
        sec
        sbc boarding_ship_step
        sta boarding_ship_b_x
        bcs board_anim_nohi
        dec boarding_ship_b_x+1
board_anim_nohi:
        lda boarding_ship_b_y
        sec
        sbc boarding_ship_step
        cmp #BOARD_SHIP_B_TARGET_Y
        bcs board_anim_sety
        lda #BOARD_SHIP_B_TARGET_Y
board_anim_sety:
        sta boarding_ship_b_y
        jsr set_boarding_ship_positions
        jsr boarding_anim_delay
        jmp board_anim_loop
board_anim_done:
        lda #<BOARD_SHIP_B_TARGET_X
        sta boarding_ship_b_x
        lda #>BOARD_SHIP_B_TARGET_X
        sta boarding_ship_b_x+1
        lda #BOARD_SHIP_B_TARGET_Y
        sta boarding_ship_b_y
        jsr set_boarding_ship_positions
        rts

boarding_anim_delay:
        ldx #4
board_delay_outer:
        ldy #0
board_delay_inner:
        dey
        bne board_delay_inner
        dex
        bne board_delay_outer
        rts

set_sprite_pointers_map:
        lda #SPRITE_PTR_BANK1_BASE
        sta SPRITE_PTR_TAB_BANK1_MAP
        lda #SPRITE_PTR_BANK1_BASE+1
        sta SPRITE_PTR_TAB_BANK1_MAP+1
        lda #SPRITE_PTR_BANK1_BASE+2
        sta SPRITE_PTR_TAB_BANK1_MAP+2
        rts

set_sprite_pointers_intro:
        lda #SPRITE_PTR_BANK1_BASE
        sta SPRITE_PTR_TAB_BANK1_INTRO
        lda #SPRITE_PTR_BANK1_BASE+1
        sta SPRITE_PTR_TAB_BANK1_INTRO+1
        lda #SPRITE_PTR_BANK1_BASE+2
        sta SPRITE_PTR_TAB_BANK1_INTRO+2
        rts

position_ship:
        lda $d010
        and #$f8
        sta $d010
        lda ship_x_hi
        beq ship_x_low
        lda $d010
        ora #7
        sta $d010
ship_x_low:
        lda ship_x
        sta $d000
        sta $d002
        sta $d004
        lda ship_y
        sta $d001
        sta $d003
        sta $d005
        rts

show_ship:
        lda #$07
        sta $d015
        rts

hide_ship:
        lda #$00
        sta $d015
        rts

askstr:  .byte 17
         .text "S]DAN SER DIT SKIB UD:"
         .byte 0
freememstr:.text "FRI HUKOMMELSE F\R KODE:"
         .byte 0
hvadstr: .text "HVAD ER DIT NAVN? "
         .byte 0
kaperstr:.text "KAPER"
         .byte 0
namebuf: .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
skip_title_screen: .byte 0
ship_x:  .byte 0
ship_x_hi:.byte 12
ship_y:  .byte 72

// Global player state (shared across include files).
crew_men:           .word 200
ship_repair_points: .word 200
treasury_rigsdaler: .word 600
grain_sacks:        .word 30
ship_cannons:       .word 20
jewel_count:        .word 0
player_points:      .word 0
// voyage_turn_counter must stay a full word: it is read/incremented as one
// everywhere (game_loop, victory checks, status displays). A stray .byte
// declaration here let its high byte alias whatever variable followed it,
// so writes to that variable (e.g. a prize payout) looked like a huge turn count.
voyage_turn_counter:.word 0
// Prize crew/rigsdaler in transit to K\benhavn (BASIC MANDPRISE/IRGSDPRI), paid out on arrival there.
pending_prize_crew_lo:       .byte 0
pending_prize_crew_hi:       .byte 0
pending_prize_rigsdaler_lo:  .byte 0
pending_prize_rigsdaler_hi:  .byte 0
// BASIC line 102: IPOINTLIM = IPOINT!+500+250*IDIF, ITURLIM = ITUR+325-12.5*IDIF (IDIF=2 at start)
point_limit:        .word 1000
turn_limit:         .word 300

kaper_code_end:
// Pure sprite data, relocated into the $8000-$87ff gap freed when the PETSCII
// intro screen was replaced by the disk-loaded bitmap.
* = $8000 "ShipSpriteData"

        // 3 sprites generated with spritemate on 01/08/2026, 12:48:00
        // Byte 64 of each sprite contains multicolor (high nibble) & color (low nibble) information
ship_sprites:
        // Red sails
        .byte $00,$00,$00,$2E,$00,$00,$6E,$00
        .byte $00,$6E,$00,$00,$E0,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0A
        // Grey mast
        .byte $10,$00,$00,$10,$00,$00,$10,$00
        .byte $00,$10,$00,$00,$10,$00,$00,$10
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$0F
        // Orange hull
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$03,$80,$00,$EF
        .byte $80,$00,$7F,$80,$00,$7F,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$00,$00,$00,$00,$03

large_boarding_ship_1:
        .byte $00,$00,$00,$00,$00,$00,$00,$00
        .byte $00,$00,$00,$10,$00,$00,$05,$00
        .byte $00,$48,$00,$00,$4A,$00,$00,$4A
        .byte $00,$00,$4A,$00,$00,$6A,$00,$10
        .byte $6A,$00,$04,$6A,$00,$21,$60,$00
        .byte $2A,$50,$00,$2A,$64,$00,$2A,$69
        .byte $00,$2A,$6A,$00,$2A,$6A,$00,$2A
        .byte $6A,$00,$2A,$6A,$00,$20,$4A,$8F

large_boarding_ship_2:
        .byte $7C,$00,$00,$7C,$00,$00,$7C,$00
        .byte $00,$40,$00,$00,$40,$00,$00,$50
        .byte $00,$00,$45,$00,$00,$68,$50,$00
        .byte $6A,$00,$00,$6A,$10,$00,$6A,$10
        .byte $00,$62,$90,$00,$4A,$90,$00,$6A
        .byte $90,$00,$6A,$90,$00,$66,$90,$00
        .byte $69,$90,$00,$6A,$50,$00,$6A,$90
        .byte $00,$6A,$94,$00,$6A,$91,$00,$8F

large_boarding_ship_3:
        .byte $01,$40,$40,$00,$95,$40,$00,$AA
        .byte $54,$00,$AA,$69,$00,$AA,$6A,$00
        .byte $AA,$6A,$00,$AA,$6A,$00,$2A,$68
        .byte $10,$0A,$60,$55,$4A,$40,$29,$82
        .byte $40,$2A,$40,$40,$0A,$90,$40,$02
        .byte $84,$40,$00,$85,$55,$00,$21,$55
        .byte $00,$01,$44,$00,$00,$FF,$00,$00
        .byte $55,$00,$00,$55,$00,$00,$15,$8F

large_boarding_ship_4:
        .byte $6A,$90,$40,$6A,$90,$10,$6A,$90
        .byte $04,$6A,$90,$01,$6A,$90,$00,$42
        .byte $90,$00,$40,$10,$00,$40,$10,$00
        .byte $40,$10,$00,$40,$10,$00,$40,$11
        .byte $55,$40,$11,$11,$40,$11,$55,$40
        .byte $11,$55,$55,$55,$55,$55,$55,$55
        .byte $44,$44,$54,$FF,$FF,$F0,$55,$55
        .byte $50,$55,$55,$50,$55,$55,$50,$8F

* = kaper_code_end

        .import source "audio.inc"
        .import source "start.inc"
        .import source "intro.inc"
        .import source "shooting.inc"
        .import source "harbour_sailing.inc"
        // harbour.inc must be imported before map.inc: map.inc ends by relocating
        // the program counter to $9000 (the embedded MapArt bitmap, inside the
        // BASIC ROM window), and anything imported after it would be linked into
        // that ROM-shadowed range and fail to execute at runtime.
        .import source "harbour.inc"
        .import source "map.inc"

// Intro picture, linked into the PRG instead of a separate KERNAL LOAD (which
// cost ~20s at 1541 speed) and RLE-packed here at assembly time. It is unpacked
// to $5c00/$6000 by decompress_intro_bitmap before every showing of the intro.
// Format: token >= $80 -> run of (token & $7f) + 1 copies of the next byte;
//         token <  $80 -> (token + 1) literal bytes follow.
.var introRaw = LoadBinary("intro.dat")
.var introPacked = List()
.var introLits = List()
.var introPos = 2                   // skip the file's 2-byte load address
.while (introPos < introRaw.getSize()) {
        .var value = introRaw.get(introPos) & 255
        .var runLen = 1
        .while (introPos + runLen < introRaw.getSize()
                && (introRaw.get(introPos + runLen) & 255) == value
                && runLen < 128) {
                .eval runLen = runLen + 1
        }
        .if (runLen >= 3 || introLits.size() == 128) {
                .if (introLits.size() > 0) {
                        .eval introPacked.add(introLits.size() - 1)
                        .for (var k = 0; k < introLits.size(); k++) {
                                .eval introPacked.add(introLits.get(k))
                        }
                        .eval introLits = List()
                }
        }
        .if (runLen >= 3) {
                .eval introPacked.add($80 + runLen - 1)
                .eval introPacked.add(value)
                .eval introPos = introPos + runLen
        } else {
                .eval introLits.add(value)
                .eval introPos = introPos + 1
        }
}
.if (introLits.size() > 0) {
        .eval introPacked.add(introLits.size() - 1)
        .for (var k = 0; k < introLits.size(); k++) {
                .eval introPacked.add(introLits.get(k))
        }
}

// RLE-pack the original map payload using the same format as the intro image.
.var mapRaw = LoadBinary("map.dat")
.var mapPacked = List()
.var mapLits = List()
.var mapPos = 2
.while (mapPos < mapRaw.getSize()) {
        .var value = mapRaw.get(mapPos) & 255
        .var runLen = 1
        .while (mapPos + runLen < mapRaw.getSize()
                && (mapRaw.get(mapPos + runLen) & 255) == value
                && runLen < 128) {
                .eval runLen = runLen + 1
        }
        .if (runLen >= 3 || mapLits.size() == 128) {
                .if (mapLits.size() > 0) {
                        .eval mapPacked.add(mapLits.size() - 1)
                        .for (var k = 0; k < mapLits.size(); k++) {
                                .eval mapPacked.add(mapLits.get(k))
                        }
                        .eval mapLits = List()
                }
        }
        .if (runLen >= 3) {
                .eval mapPacked.add($80 + runLen - 1)
                .eval mapPacked.add(value)
                .eval mapPos = mapPos + runLen
        } else {
                .eval mapLits.add(value)
                .eval mapPos = mapPos + 1
        }
}
.if (mapLits.size() > 0) {
        .eval mapPacked.add(mapLits.size() - 1)
        .for (var k = 0; k < mapLits.size(); k++) {
                .eval mapPacked.add(mapLits.get(k))
        }
}

.var mapBitmapRaw = LoadBinary("map.dat")
.var mapBitmapPacked = List()
.var mapBitmapLits = List()
.var mapBitmapPos = 2
.while (mapBitmapPos < 2 + 8000) {
        .var value = mapBitmapRaw.get(mapBitmapPos) & 255
        .var runLen = 1
        .while (mapBitmapPos + runLen < 2 + 8000
                && (mapBitmapRaw.get(mapBitmapPos + runLen) & 255) == value
                && runLen < 128) {
                .eval runLen = runLen + 1
        }
        .if (runLen >= 3 || mapBitmapLits.size() == 128) {
                .if (mapBitmapLits.size() > 0) {
                        .eval mapBitmapPacked.add(mapBitmapLits.size() - 1)
                        .for (var k = 0; k < mapBitmapLits.size(); k++) {
                                .eval mapBitmapPacked.add(mapBitmapLits.get(k))
                        }
                        .eval mapBitmapLits = List()
                }
        }
        .if (runLen >= 3) {
                .eval mapBitmapPacked.add($80 + runLen - 1)
                .eval mapBitmapPacked.add(value)
                .eval mapBitmapPos = mapBitmapPos + runLen
        } else {
                .eval mapBitmapLits.add(value)
                .eval mapBitmapPos = mapBitmapPos + 1
        }
}
.if (mapBitmapLits.size() > 0) {
        .eval mapBitmapPacked.add(mapBitmapLits.size() - 1)
        .for (var k = 0; k < mapBitmapLits.size(); k++) {
                .eval mapBitmapPacked.add(mapBitmapLits.get(k))
        }
}

.var mapScreenRaw = LoadBinary("map.dat")
.var mapScreenPacked = List()
.var mapScreenLits = List()
.var mapScreenPos = 2 + 8000
.while (mapScreenPos < 2 + 8000 + 1000) {
        .var value = mapScreenRaw.get(mapScreenPos) & 255
        .var runLen = 1
        .while (mapScreenPos + runLen < 2 + 8000 + 1000
                && (mapScreenRaw.get(mapScreenPos + runLen) & 255) == value
                && runLen < 128) {
                .eval runLen = runLen + 1
        }
        .if (runLen >= 3 || mapScreenLits.size() == 128) {
                .if (mapScreenLits.size() > 0) {
                        .eval mapScreenPacked.add(mapScreenLits.size() - 1)
                        .for (var k = 0; k < mapScreenLits.size(); k++) {
                                .eval mapScreenPacked.add(mapScreenLits.get(k))
                        }
                        .eval mapScreenLits = List()
                }
        }
        .if (runLen >= 3) {
                .eval mapScreenPacked.add($80 + runLen - 1)
                .eval mapScreenPacked.add(value)
                .eval mapScreenPos = mapScreenPos + runLen
        } else {
                .eval mapScreenLits.add(value)
                .eval mapScreenPos = mapScreenPos + 1
        }
}
.if (mapScreenLits.size() > 0) {
        .eval mapScreenPacked.add(mapScreenLits.size() - 1)
        .for (var k = 0; k < mapScreenLits.size(); k++) {
                .eval mapScreenPacked.add(mapScreenLits.get(k))
        }
}

// Sits in the RAM under the BASIC ROM, in the gap between MapArt and the
// shooting screen data; decompress_intro_bitmap banks BASIC out to read it.
.errorif (INTRO_PACKED_ADDR + introPacked.size() > $c000), "Packed intro data overruns $c000"
* = INTRO_PACKED_ADDR "IntroBitmapPacked"
intro_rle_data:
        .fill introPacked.size(), introPacked.get(i)

.errorif (MAP_PACKED_ADDR + mapBitmapPacked.size() + mapScreenPacked.size() > $b340), "Packed map data overruns under-BASIC RAM"
* = $9000 "MapBitmapPacked"
map_bitmap_rle_data:
        .fill mapBitmapPacked.size(), mapBitmapPacked.get(i)
* = $9000 + mapBitmapPacked.size() "MapScreenPacked"
map_screen_rle_data:
        .fill mapScreenPacked.size(), mapScreenPacked.get(i)

