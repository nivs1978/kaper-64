// Shooting minigame screen display.
// Screen data from C:\source\Kaper64 BASIC\shooting.asm (petmate export, 40x25 C64 upper).
// Border 14 (light blue), background 6 (dark blue).

run_shooting_minigame:
        jsr hide_vic_screen
        lda #$df
        and $d011
        sta $d011
        lda #14
        sta $d020
        lda #6
        sta $d021
        lda #0
        sta $d015
        jsr init_shooting_state
        lda #0
        sta battle_return_to_main_choice
        sta shoot_elev
        sta shoot_side
        jsr load_shooting_ship
        lda #<crosshair_sprite
        sta intro_src_ptr
        lda #>crosshair_sprite
        sta intro_src_ptr+1
        lda #<$5100
        sta intro_dst_ptr
        lda #>$5100
        sta intro_dst_ptr+1
        ldy #63
shooting_crosshair_copy:
        lda (intro_src_ptr),y
        sta (intro_dst_ptr),y
        dey
        bpl shooting_crosshair_copy
        lda #SPRITE_PTR_BANK1_BASE+4
        sta SPRITE_PTR_TAB_BANK1_INTRO
        lda #10
        sta $d027
        jsr update_crosshair_pos
        jsr redraw_shooting_view
        lda #$1f
        sta $d015
        jsr show_vic_screen
        lda #0
        sta $c6
xhair_loop:
        jsr $ffe4
        beq xhair_loop
        cmp #$30
        beq xhair_abort
        cmp #'F'
        beq xhair_done
        cmp #'f'
        beq xhair_done
        cmp #$11
        beq xhair_down
        cmp #$91
        beq xhair_up
        cmp #$9d
        beq xhair_left
        cmp #$1d
        beq xhair_right
        cmp #'2'
        beq xhair_down
        cmp #'8'
        beq xhair_up
        cmp #'4'
        beq xhair_left
        cmp #'6'
        beq xhair_right
        jmp xhair_loop
xhair_abort:
        lda #0
        sta $d015
        jsr clear_text_notice_screen
        rts
xhair_done:
        jsr resolve_shot_round
        lda battle_return_to_main_choice
        cmp #2
        beq xhair_exit_battle
        jsr update_shooting_screen
        jsr draw_shot_markers
        jsr show_shot_prompt
fire_wait_key:
        jsr $ffe4
        beq fire_wait_key
        jsr redraw_shooting_view
        jmp xhair_loop
xhair_exit_battle:
        rts

xhair_up:
        dec shoot_elev
        jsr clamp_elev
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down:
        inc shoot_elev
        jsr clamp_elev
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_left:
        dec shoot_side
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_right:
        inc shoot_side
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_up_left:
        dec shoot_elev
        dec shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_up_right:
        dec shoot_elev
        inc shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down_left:
        inc shoot_elev
        dec shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

xhair_down_right:
        inc shoot_elev
        inc shoot_side
        jsr clamp_elev
        jsr clamp_side
        jsr update_crosshair_pos
        jmp xhair_loop

update_shooting_screen:
        ldx #0
upd_fld_clr:
        lda shoot_fld_clr_lo,x
        sta intro_dst_ptr
        lda shoot_fld_clr_hi,x
        sta intro_dst_ptr+1
        ldy #5
        lda #15
upd_fld_clr_loop:
        sta (intro_dst_ptr),y
        dey
        bpl upd_fld_clr_loop
        inx
        cpx #7
        bne upd_fld_clr

        ldx #6
        ldy #17
        fnum(shoot_dist_lo)

        lda ship_cannons
        ldx #14
        ldy #19
        jsr print_fight_byte_stat

        ldx #14
        ldy #21
        fnum(crew_men)

        ldx #14
        ldy #23
        fnum(ship_repair_points)

        lda enemy_cannons_lo
        ldx #27
        ldy #19
        jsr print_fight_byte_stat

        ldx #27
        ldy #21
        fnum(enemy_crew_lo)

        ldx #27
        ldy #23
        fnum(enemy_ship_hp_lo)

        ldx #3
        ldy #20
        jsr set_screen_ptr_xy
        ldy #0
        lda #$20
        sta (dst_ptr),y
        iny
        sta (dst_ptr),y
        lda shoot_wind_str
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_word_decimal

        ldx #0
upd_wind_reset:
        lda shoot_wind_clr_lo,x
        sta intro_dst_ptr
        lda #>$db4a
        sta intro_dst_ptr+1
        ldy #0
        lda #15
        sta (intro_dst_ptr),y
        iny
        sta (intro_dst_ptr),y
        iny
        sta (intro_dst_ptr),y
        inx
        cpx #3
        bne upd_wind_reset

        ldx shoot_wind_ver
        lda shoot_wind_clr_lo,x
        sta intro_dst_ptr
        lda #>$db4a
        sta intro_dst_ptr+1
        ldy shoot_wind_hor
        lda #4
        sta (intro_dst_ptr),y
        rts

init_shooting_state:
        lda #0
        sta shot_roll
        sta shot_power
        sta shot_penalty

        // BASIC line 1850: IAFST = 500 + INT(RND * 500).
        lda $dc04
        asl
        clc
        adc #<500
        sta shoot_dist_lo
        lda #>500
        adc #0
        sta shoot_dist_hi
        cmp #>1000
        bcc init_shoot_dist_done
        bne init_shoot_dist_wrap
        lda shoot_dist_lo
        cmp #<1000
        bcc init_shoot_dist_done
init_shoot_dist_wrap:
        sec
        lda shoot_dist_lo
        sbc #11
        sta shoot_dist_lo
        lda shoot_dist_hi
        sbc #0
        sta shoot_dist_hi
init_shoot_dist_done:

        // BASIC line 1950: each wind axis is -10, 0, or +10.
        jsr random_mod_8
        and #$03
        cmp #3
        bne init_shoot_wind_hor_ok
        lda #1
init_shoot_wind_hor_ok:
        sta shoot_wind_hor

        jsr random_mod_8
        and #$03
        cmp #3
        bne init_shoot_wind_ver_ok
        lda #1
init_shoot_wind_ver_ok:
        sta shoot_wind_ver

        // BASIC line 1960: strength is 1..10, unless both axes are calm.
        jsr random_mod_16
        cmp #10
        bcc init_shoot_wind_strength_ok
        sec
        sbc #10
init_shoot_wind_strength_ok:
        clc
        adc #1
        sta shoot_wind_str
        lda shoot_wind_hor
        cmp #1
        bne init_shoot_state_done
        lda shoot_wind_ver
        cmp #1
        bne init_shoot_state_done
        lda #0
        sta shoot_wind_str
init_shoot_state_done:
        rts

// color RAM low bytes for wind compass rows 21/22/23 at col 2
// high byte is $DB for all three ($DB4A, $DB72, $DB9A)
shoot_wind_clr_lo:
        .byte <($db4a + 0*40)
        .byte <($db4a + 1*40)
        .byte <($db4a + 2*40)

shoot_dist_lo:  .byte 0
shoot_dist_hi:  .byte 0
shoot_wind_str: .byte 0
shoot_wind_hor: .byte 0     // 0=left 1=center 2=right
shoot_wind_ver: .byte 0     // 0=up 1=center 2=down

// color RAM start addresses for the 7 stat value fields ($D800 + row*40 + col)
shoot_fld_clr_lo:
        .byte <$daae    // AFST     row 17 col 6
        .byte <$db06    // own SKYTS row 19 col 14
        .byte <$db56    // own MAND  row 21 col 14
        .byte <$dba6    // own REP   row 23 col 14
        .byte <$db13    // foe SKYTS row 19 col 27
        .byte <$db63    // foe MAND  row 21 col 27
        .byte <$dbb3    // foe REP   row 23 col 27
shoot_fld_clr_hi:
        .byte >$daae,>$db06,>$db56,>$dba6,>$db13,>$db63,>$dbb3

// Clamp shoot_elev to -30..+30 (IELEVA bounds, BASIC lines 2170-2180)
clamp_elev:
        lda shoot_elev
        bmi clamp_elev_neg
        cmp #31
        bcc clamp_elev_done
        lda #30
        sta shoot_elev
        rts
clamp_elev_neg:
        cmp #$e2            // $E2 = -30 in two's complement
        bcs clamp_elev_done
        lda #$e2
        sta shoot_elev
clamp_elev_done:
        rts

// Clamp shoot_side to -50..+50 (ISIDE bounds, BASIC lines 2190-2200)
clamp_side:
        lda shoot_side
        bmi clamp_side_neg
        cmp #51
        bcc clamp_side_done
        lda #50
        sta shoot_side
        rts
clamp_side_neg:
        cmp #$ce            // $CE = -50 in two's complement
        bcs clamp_side_done
        lda #$ce
        sta shoot_side
clamp_side_done:
        rts

// Update sprite 0 X/Y from shoot_side/elev.
// X = 244 + shoot_side (signed), Y = 90 + shoot_elev (signed).
update_crosshair_pos:
        lda shoot_side
        bmi ucp_neg_side
        clc
        adc #244            // positive side: carry set if result > 255
        sta $d000
        bcc ucp_clear_xhi
        lda $d010
        ora #$01            // set X_hi for sprite 0
        sta $d010
        jmp ucp_y
ucp_neg_side:
        clc
        adc #244            // negative side: result always < 256
        sta $d000
ucp_clear_xhi:
        lda $d010
        and #$fe
        sta $d010
ucp_y:
        lda #90
        clc
        adc shoot_elev      // range 60..120; ignore carry
        sta $d001
        rts

redraw_shooting_view:
        lda #<shooting_screen_data
        sta intro_src_ptr
        lda #>shooting_screen_data
        sta intro_src_ptr+1
        lda #<INTRO_SCREEN_RAM
        sta intro_dst_ptr
        lda #>INTRO_SCREEN_RAM
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        lda #<shooting_color_data
        sta intro_src_ptr
        lda #>shooting_color_data
        sta intro_src_ptr+1
        lda #<$d800
        sta intro_dst_ptr
        lda #>$d800
        sta intro_dst_ptr+1
        jsr copy_1000_bytes

        jsr update_shooting_screen
        jsr update_crosshair_pos
        lda #$1f
        sta $d015
        rts

// The enemy ship uses four multicolor sprites in blocks 64-67. Sprite 0 is
// reserved for the crosshair, which has priority over sprites 1-4.
load_shooting_ship:
        lda #SPRITE_PTR_BANK1_BASE
        sta SPRITE_PTR_TAB_BANK1_INTRO+1
        lda #SPRITE_PTR_BANK1_BASE+1
        sta SPRITE_PTR_TAB_BANK1_INTRO+2
        lda #SPRITE_PTR_BANK1_BASE+2
        sta SPRITE_PTR_TAB_BANK1_INTRO+3
        lda #SPRITE_PTR_BANK1_BASE+3
        sta SPRITE_PTR_TAB_BANK1_INTRO+4

        lda #4
        sta $d025
        lda #3
        sta $d026
        lda #15
        sta $d028
        sta $d029
        sta $d02a
        sta $d02b

        lda #$1e
        sta $d01c
        lda #$00
        sta $d010
        sta $d017
        sta $d01d
        lda #$1e
        sta $d01b

        // BASIC's default enemy graphic position is IAKTX=225, IAKTY=70;
        // shift the sprite 2 pixels right and 10 pixels down.
        lda #227
        sta $d002
        sta $d006
        lda #251
        sta $d004
        sta $d008

        lda #80
        sta $d003
        sta $d005
        lda #101
        sta $d007
        sta $d009

        ldx #0
shooting_ship_copy:
        lda large_boarding_ship_1,x
        sta SPRITE_DATA_BANK1,x
        lda large_boarding_ship_2,x
        sta SPRITE_DATA_BANK1+64,x
        lda large_boarding_ship_3,x
        sta SPRITE_DATA_BANK1+128,x
        lda large_boarding_ship_4,x
        sta SPRITE_DATA_BANK1+192,x
        inx
        cpx #64
        bne shooting_ship_copy
        rts

draw_shot_markers:
        // Original pixel effects: own (IXPL,IYPL), enemy (IAKTX,IAKTY).
        // These coordinates are their character-cell equivalents.
        lda shot_marker_is_splash
        beq draw_shot_markers_hit
        lda #<shot_splash_marker_msg
        sta src_ptr
        lda #>shot_splash_marker_msg
        sta src_ptr+1
        jmp draw_shot_markers_draw
draw_shot_markers_hit:
        lda #<shot_marker_msg
        sta src_ptr
        lda #>shot_marker_msg
        sta src_ptr+1
draw_shot_markers_draw:
        lda shot_marker_is_splash
        beq draw_shot_markers_landing
        jsr draw_shot_splashes
draw_shot_markers_landing:
        ldx shot_own_marker_x
        ldy shot_own_marker_y
        jsr draw_shot_marker_at
        ldx shot_enemy_marker_x
        ldy shot_enemy_marker_y
        jsr draw_shot_marker_at
        lda shot_marker_is_splash
        beq draw_shot_markers_done
        jsr animate_enemy_splash
draw_shot_markers_done:
        rts

animate_enemy_splash:
        ldx shot_enemy_marker_x
        ldy shot_enemy_marker_y
        lda #$3c
        jsr draw_shot_char_at
        ldx #30
        jsr shooting_wait_frames
        ldx shot_enemy_marker_x
        ldy shot_enemy_marker_y
        lda #$3e
        jsr draw_shot_char_at
        ldx #30
        jsr shooting_wait_frames
        ldx shot_enemy_marker_x
        ldy shot_enemy_marker_y
        lda #' '
        jsr draw_shot_char_at
        rts

draw_shot_char_at:
        sta shot_char_value
        stx shot_marker_draw_x
        sty shot_marker_draw_y
        jsr set_screen_ptr_xy
        ldy #0
        lda shot_char_value
        sta (dst_ptr),y
        ldy shot_marker_draw_y
        lda screen_row_addr_lo,y
        sta intro_dst_ptr
        lda screen_row_addr_hi,y
        clc
        adc #$94
        sta intro_dst_ptr+1
        ldx shot_marker_draw_x
        txa
        clc
        adc intro_dst_ptr
        sta intro_dst_ptr
        bcc draw_shot_char_color_ptr_ok
        inc intro_dst_ptr+1
draw_shot_char_color_ptr_ok:
        ldy #0
        lda #1
        sta (intro_dst_ptr),y
        rts

shooting_wait_frames:
        stx shooting_wait_count
        lda $a2
        sta shooting_wait_previous
shooting_wait_same:
        lda $a2
        cmp shooting_wait_previous
        beq shooting_wait_same
        sta shooting_wait_previous
        dec shooting_wait_count
        bne shooting_wait_same
        rts

draw_shot_marker_at:
        stx shot_marker_draw_x
        sty shot_marker_draw_y
        jsr write_zero_terminated_screen

        ldy shot_marker_draw_y
        lda screen_row_addr_lo,y
        sta dst_ptr
        lda screen_row_addr_hi,y
        clc
        adc #$94
        sta dst_ptr+1
        lda shot_marker_draw_x
        clc
        adc dst_ptr
        sta dst_ptr
        bcc draw_shot_marker_color_ptr_ok
        inc dst_ptr+1
draw_shot_marker_color_ptr_ok:
        ldy #0
        lda #1
        sta (dst_ptr),y
        rts

draw_shot_splashes:
        lda shot_splash_count
        sta shot_splash_remaining
        beq draw_shot_splashes_done
draw_shot_splashes_loop:
        // BASIC ICKX=20..129, ICKY=70..89: the player's lower-left frame.
        // Converted to screen cells, keep the asterisks in columns 3..16
        // and rows 9..12, inside the player's frame.
        jsr random_mod_16
        and #$0d
        clc
        adc #3
        tax
        jsr random_mod_8
        and #$03
        clc
        adc #9
        tay
        jsr draw_shot_marker_at
        dec shot_splash_remaining
        bne draw_shot_splashes_loop
draw_shot_splashes_done:
        rts

resolve_shot_round:
        jsr sound_cannon
        jsr random_mod_16
        sta shot_roll

        lda ship_cannons
        lsr
        clc
        adc shot_roll
        sta shot_power

        lda shoot_side
        bpl rsr_side_abs_done
        eor #$ff
        clc
        adc #1
rsr_side_abs_done:
        lsr
        lsr
        lsr
        sta shot_penalty

        lda shoot_elev
        bpl rsr_elev_abs_done
        eor #$ff
        clc
        adc #1
rsr_elev_abs_done:
        lsr
        lsr
        lsr
        clc
        adc shot_penalty
        adc shoot_wind_str
        sta shot_penalty

        lda shot_power
        sec
        sbc shot_penalty
        sta shot_power

        jsr calculate_shot_deflection

        // BASIC line 2740: IAFV = IAFST - (700 + (ISTYRKE * IVINDL) - (10 * IELEVA) + rnd-rnd).
        // A positive value means the shot went over; below -50 means it fell short.
        lda shoot_dist_lo
        sec
        sbc #<700
        sta shot_dist_error_lo
        lda shoot_dist_hi
        sbc #>700
        sta shot_dist_error_hi

        lda shoot_elev
        bmi rsr_dist_elev_neg
rsr_dist_elev_pos:
        ldx shoot_elev
        beq rsr_dist_elev_done
rsr_dist_elev_pos_loop:
        clc
        lda shot_dist_error_lo
        adc #10
        sta shot_dist_error_lo
        bcc rsr_dist_elev_pos_no_carry
        inc shot_dist_error_hi
rsr_dist_elev_pos_no_carry:
        dex
        bne rsr_dist_elev_pos_loop
        jmp rsr_dist_elev_done
rsr_dist_elev_neg:
        eor #$ff
        clc
        adc #1
        tax
        beq rsr_dist_elev_done
rsr_dist_elev_neg_loop:
        sec
        lda shot_dist_error_lo
        sbc #10
        sta shot_dist_error_lo
        bcs rsr_dist_elev_neg_no_borrow
        dec shot_dist_error_hi
rsr_dist_elev_neg_no_borrow:
        dex
        bne rsr_dist_elev_neg_loop
rsr_dist_elev_done:

        // IVINDL is -10, 0, or +10 for wind from above, calm, or below.
        // IAFV subtracts the wind term, so upward wind adds to IAFV and
        // downward wind subtracts from it.
        lda shoot_wind_ver
        beq rsr_dist_wind_up
        cmp #2
        bne rsr_dist_wind_done
        ldx shoot_wind_str
rsr_dist_wind_down_loop:
        beq rsr_dist_wind_done
        sec
        lda shot_dist_error_lo
        sbc #10
        sta shot_dist_error_lo
        bcs rsr_dist_wind_down_no_borrow
        dec shot_dist_error_hi
rsr_dist_wind_down_no_borrow:
        dex
        jmp rsr_dist_wind_down_loop
rsr_dist_wind_up:
        ldx shoot_wind_str
rsr_dist_wind_up_loop:
        beq rsr_dist_wind_done
        clc
        lda shot_dist_error_lo
        adc #10
        sta shot_dist_error_lo
        bcc rsr_dist_wind_up_no_carry
        inc shot_dist_error_hi
rsr_dist_wind_up_no_carry:
        dex
        jmp rsr_dist_wind_up_loop
rsr_dist_wind_done:

        // BASIC INT(50*RND) - INT(50*RND), applied inside the parentheses.
        jsr rsr_random_50
        sta shot_roll
        sec
        lda shot_dist_error_lo
        sbc shot_roll
        sta shot_dist_error_lo
        bcs rsr_dist_noise_sub_no_borrow
        dec shot_dist_error_hi
rsr_dist_noise_sub_no_borrow:
        jsr rsr_random_50
        sta shot_roll
        clc
        lda shot_dist_error_lo
        adc shot_roll
        sta shot_dist_error_lo
        bcc rsr_dist_noise_add_no_carry
        inc shot_dist_error_hi
rsr_dist_noise_add_no_carry:

        jsr classify_and_mark_shot_landing
        lda shot_landing_y
        cmp #1
        bne rsr_miss
        lda shot_landing_x
        cmp #1
        bne rsr_miss
        jmp rsr_hit
rsr_miss:
        jsr sound_splash
        lda #1
        sta shot_marker_is_splash
        jsr apply_enemy_cannon_result
        jmp rsr_damage_player

rsr_hit:
        lda #0
        sta shot_marker_is_splash
        jsr apply_enemy_cannon_result
        lda enemy_ship_hp_hi
        ora enemy_ship_hp_lo
        beq rsr_enemy_defeated
        jmp rsr_damage_player

rsr_enemy_defeated:
        jsr clear_enemy_ship_surrender_area
        lda #$00
        sta $d015
        // Enemy frame interior: column 5, one row below the top.
        lda #5
        ldx #1
        ldy #5
        jsr show_surrender_flag
rsr_enemy_report:
        jsr sound_surrender
        jsr handle_enemy_surrender
        lda #2
        sta battle_return_to_main_choice
        rts

// BASIC 3030-3070: hull destroyed, so the ship sinks before the report.
// BASIC 2900 jumps past 2950, so a sinking never strikes the colours.
rsr_enemy_sunk:
        // Clear the shot markers first, then sink the sprite ship.
        jsr redraw_shooting_view
        lda #$1e
        sta $d015
        jsr sink_enemy_ship
        jmp rsr_enemy_report

rsr_check_enemy_hp:
        lda enemy_ship_hp_hi
        ora enemy_ship_hp_lo
        bne rsr_damage_player
        jmp rsr_enemy_sunk

rsr_damage_player:
        jsr random_mod_8
        clc
        adc #1
        adc shoot_wind_str
        sta damage_lo
        lda #0
        sta damage_hi

        lda damage_lo
        cmp #11
        bcc rsr_splash_count_ok
        lda #10
rsr_splash_count_ok:
        sta shot_splash_count

        sec
        lda ship_repair_points
        sbc damage_lo
        sta ship_repair_points
        lda ship_repair_points+1
        sbc damage_hi
        sta ship_repair_points+1
        bcs rsr_repair_ok
        lda #0
        sta ship_repair_points
        sta ship_repair_points+1
rsr_repair_ok:

        lda damage_lo
        lsr
        clc
        adc #1
        sta damage_lo
        lda #0
        sta damage_hi

        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        bcs rsr_player_crew_ok
        lda #0
        sta crew_men
        sta crew_men+1
rsr_player_crew_ok:
        jsr check_ship_survival
        rts

apply_enemy_cannon_result:
        lda shot_dist_error_hi
        bpl aec_done
        lda shot_dist_error_lo
        cmp #$ce
        bcc aec_done
        lda ship_cannons
        asl
        sta battle_product_lo
        lda ship_cannons+1
        rol
        sta battle_product_hi
        lda difficulty_level
        sta battle_divisor_lo
        lda #0
        sta battle_divisor_hi
        jsr divide_battle_product
        sec
        lda enemy_ship_hp_lo
        sbc battle_product_lo
        sta enemy_ship_hp_lo
        lda enemy_ship_hp_hi
        sbc battle_product_hi
        sta enemy_ship_hp_hi
        bcc aec_clear
        clc
        lda enemy_ship_hp_lo
        adc shot_dist_error_lo
        sta enemy_ship_hp_lo
        lda enemy_ship_hp_hi
        adc shot_dist_error_hi
        sta enemy_ship_hp_hi
        bpl aec_done
aec_clear:
        lda #0
        sta enemy_ship_hp_lo
        sta enemy_ship_hp_hi
aec_done:
        rts

apply_cannon_player_damage:
        jsr random_mod_10
        cmp #4
        bcc acpd_crew
        lda enemy_cannons_lo
        lsr
        clc
        adc #1
        sta damage_lo
        lda #0
        sta damage_hi
        sec
        lda ship_repair_points
        sbc damage_lo
        sta ship_repair_points
        lda ship_repair_points+1
        sbc damage_hi
        sta ship_repair_points+1
acpd_crew:
        jsr random_mod_10
        cmp #4
        bcc acpd_done
        lda enemy_cannons_lo
        lsr
        clc
        adc #1
        sta damage_lo
        lda #0
        sta damage_hi
        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        sta crew_men+1
acpd_done:
        rts

rsr_random_50:
        jsr random_mod_64
        cmp #50
        bcs rsr_random_50
        rts

classify_and_mark_shot_landing:
        lda #1
        sta shot_landing_y
        lda shot_dist_error_hi
        bpl classify_landing_iret_positive
        lda shot_dist_error_lo
        cmp #$ce
        bcs classify_landing_x
        lda #2
        sta shot_landing_y
        jmp classify_landing_x
classify_landing_iret_positive:
        lda #0
        sta shot_landing_y
classify_landing_x:
        lda #1
        sta shot_landing_x
        lda shot_iret
        bpl classify_landing_iret_positive_x
        eor #$ff
        clc
        adc #1
        cmp shot_hit_tolerance
        bcc classify_landing_markers
        beq classify_landing_markers
        lda #2
        sta shot_landing_x
        jmp classify_landing_markers
classify_landing_iret_positive_x:
        cmp shot_hit_tolerance
        bcc classify_landing_markers
        beq classify_landing_markers
        lda #0
        sta shot_landing_x
classify_landing_markers:
        jmp set_enemy_impact_marker

set_enemy_impact_marker:
        lda shot_landing_x
        cmp #1
        beq set_enemy_impact_center_x
        cmp #2
        beq set_enemy_impact_right_x
        lda #4
        sta shot_own_marker_x
        lda #23
        sta shot_enemy_marker_x
        jmp set_enemy_impact_y
set_enemy_impact_right_x:
        lda #16
        sta shot_own_marker_x
        lda #35
        sta shot_enemy_marker_x
        jmp set_enemy_impact_y
set_enemy_impact_center_x:
        lda #10
        sta shot_own_marker_x
        lda #28
        sta shot_enemy_marker_x
set_enemy_impact_y:
        lda shot_landing_y
        cmp #1
        beq set_enemy_impact_center_y
        cmp #2
        beq set_enemy_impact_low
        lda #3
        sta shot_own_marker_y
        lda #5
        sta shot_enemy_marker_y
        rts
set_enemy_impact_center_y:
        lda #4
        sta shot_own_marker_y
        lda #6
        sta shot_enemy_marker_y
        rts
set_enemy_impact_low:
        lda #5
        sta shot_own_marker_y
        lda #7
        sta shot_enemy_marker_y
        rts

calculate_shot_deflection:
        // BASIC 2630: IRET = wind drift + random drift - (ISIDE / 5).
        // Wind direction is -10, 0, or +10; the scaled terms are kept in
        // the same small signed range as the original integer calculation.
        lda shoot_wind_hor
        beq calc_deflection_left
        cmp #2
        beq calc_deflection_right
        lda #0
        sta shot_iret
        jmp calc_deflection_side
calc_deflection_left:
        lda shoot_wind_str
        eor #$ff
        clc
        adc #1
        sta shot_iret
        jsr calc_deflection_random_left
        jmp calc_deflection_side
calc_deflection_right:
        lda shoot_wind_str
        sta shot_iret
        jsr calc_deflection_random_right
calc_deflection_side:
        lda shoot_side
        bpl calc_deflection_side_positive
        eor #$ff
        clc
        adc #1
calc_deflection_side_positive:
        jsr signed_divide_by_five
        sta shot_roll
        lda shoot_side
        bpl calc_deflection_sub_side
        lda shot_iret
        clc
        adc shot_roll
        sta shot_iret
        jmp calc_deflection_done
calc_deflection_sub_side:
        lda shot_iret
        sec
        sbc shot_roll
        sta shot_iret
calc_deflection_done:
        lda shot_iret
        bmi calc_deflection_clamp_negative
        cmp #21
        bcc calc_deflection_tolerance
        lda #20
        sta shot_iret
        jmp calc_deflection_tolerance
calc_deflection_clamp_negative:
        lda shot_iret
        cmp #$ec
        bcs calc_deflection_tolerance
        lda #$ec
        sta shot_iret
calc_deflection_tolerance:
        lda #2
        sta shot_hit_tolerance
        lda difficulty_level
        cmp #5
        bcc calc_deflection_done_tolerance
        lda #1
        sta shot_hit_tolerance
        lda difficulty_level
        cmp #7
        bcc calc_deflection_done_tolerance
        lda #0
        sta shot_hit_tolerance
calc_deflection_done_tolerance:
        rts

calc_deflection_random_left:
        jsr random_mod_8
        and #1
        beq calc_deflection_random_done
        lda shot_iret
        sec
        sbc #1
        sta shot_iret
calc_deflection_random_done:
        rts

calc_deflection_random_right:
        jsr random_mod_8
        and #1
        beq calc_deflection_random_done
        lda shot_iret
        clc
        adc #1
        sta shot_iret
        rts

signed_divide_by_five:
        ldx #0
calc_divide_by_five_loop:
        cmp #5
        bcc calc_divide_by_five_done
        sec
        sbc #5
        inx
        jmp calc_divide_by_five_loop
calc_divide_by_five_done:
        txa
        rts

show_shot_prompt:
        ldx #17
        ldy #15
        wstr(shooting_press_key_msg)
        rts

// Crosshair sprite: vertical line at pixel 11, horizontal at row 10 (24x21 singlecolor).
crosshair_sprite:
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $FF,$FF,$FE
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00, $00,$10,$00
        .byte $0A           // unused padding (VIC reads color from $D027)

shoot_elev: .byte 0         // -30..+30  (IELEVA equivalent)
shoot_side: .byte 0         // -50..+50  (ISIDE equivalent)
shot_own_marker_x: .byte 0
shot_own_marker_y: .byte 0
shot_enemy_marker_x: .byte 0
shot_enemy_marker_y: .byte 0
shot_marker_is_splash: .byte 0
shot_splash_count: .byte 0
shot_splash_remaining: .byte 0
shot_landing_x: .byte 0
shot_landing_y: .byte 0
shot_marker_draw_x: .byte 0
shot_marker_draw_y: .byte 0
shot_iret: .byte 0
shot_hit_tolerance: .byte 0
shot_dist_error_lo: .byte 0
shot_dist_error_hi: .byte 0
shot_roll: .byte 0
shot_power: .byte 0
shot_penalty: .byte 0
shot_char_value: .byte 0
shooting_wait_count: .byte 0
shooting_wait_previous: .byte 0

shot_marker_msg:
        .text "X"
        .byte 0

shot_splash_marker_msg:
        .text "*"
        .byte 0

shooting_press_key_msg:
        .text "TRYK EN TAST          "
        .byte 0

// The sight-panel ship occupies rows 3-9, columns 24-33 of the shooting screen.
.const SINK_TOP_ROW = 3
.const SINK_ROWS = 7
.const SINK_LEFT_COL = 24
.const SINK_WIDTH = 10

clear_enemy_ship_surrender_area:
        ldx #SINK_ROWS-1
clear_enemy_ship_row:
        lda sink_scr_lo,x
        sta dst_ptr
        lda sink_scr_hi,x
        sta dst_ptr+1
        ldy #SINK_WIDTH-1
        lda #$20
clear_enemy_ship_screen:
        sta (dst_ptr),y
        dey
        bpl clear_enemy_ship_screen

        lda sink_col_lo,x
        sta dst_ptr
        lda sink_col_hi,x
        sta dst_ptr+1
        ldy #SINK_WIDTH-1
        lda #$00
clear_enemy_ship_colour:
        sta (dst_ptr),y
        dey
        bpl clear_enemy_ship_colour

        dex
        bpl clear_enemy_ship_row
        rts

sink_enemy_ship:
        ldx #60
        lda #21
        sta sink_steps
sink_step_loop:
        lda #0
        sta SPRITE_DATA_BANK1+128,x
        sta SPRITE_DATA_BANK1+192,x
        inx
        sta SPRITE_DATA_BANK1+128,x
        sta SPRITE_DATA_BANK1+192,x
        inx
        sta SPRITE_DATA_BANK1+128,x
        sta SPRITE_DATA_BANK1+192,x
        txa
        sec
        sbc #5
        tax

        inc $d003
        inc $d005
        inc $d007
        inc $d009
        jsr sink_wait_fast
        dec sink_steps
        bne sink_step_loop

        ldx #60
        lda #21
        sta sink_steps
sink_top_step_loop:
        lda #0
        sta SPRITE_DATA_BANK1,x
        sta SPRITE_DATA_BANK1+64,x
        inx
        sta SPRITE_DATA_BANK1,x
        sta SPRITE_DATA_BANK1+64,x
        inx
        sta SPRITE_DATA_BANK1,x
        sta SPRITE_DATA_BANK1+64,x
        txa
        sec
        sbc #5
        tax

        inc $d003
        inc $d005
        inc $d007
        inc $d009
        jsr sink_wait_fast
        dec sink_steps
        bne sink_top_step_loop

        lda #0
        sta $d015
        rts

// Each step drops the ship one row; the bottom row goes under and the top blanks.
sink_shift_down:
        ldx #SINK_ROWS-1
sink_row_loop:
        lda sink_scr_lo,x
        sta dst_ptr
        lda sink_scr_hi,x
        sta dst_ptr+1
        lda sink_scr_lo-1,x
        sta src_ptr
        lda sink_scr_hi-1,x
        sta src_ptr+1
        ldy #SINK_WIDTH-1
sink_copy_screen:
        lda (src_ptr),y
        sta (dst_ptr),y
        dey
        bpl sink_copy_screen

        lda sink_col_lo,x
        sta dst_ptr
        lda sink_col_hi,x
        sta dst_ptr+1
        lda sink_col_lo-1,x
        sta src_ptr
        lda sink_col_hi-1,x
        sta src_ptr+1
        ldy #SINK_WIDTH-1
sink_copy_colour:
        lda (src_ptr),y
        sta (dst_ptr),y
        dey
        bpl sink_copy_colour

        dex
        bne sink_row_loop

        lda sink_scr_lo
        sta dst_ptr
        lda sink_scr_hi
        sta dst_ptr+1
        ldy #SINK_WIDTH-1
        lda #$20
sink_blank_top:
        sta (dst_ptr),y
        dey
        bpl sink_blank_top
        rts

// Five times faster than the original 15-tick sinking delay.
sink_wait_fast:
        lda #3
        sta sink_delay
sink_wait_tick:
        lda $a2
        sta sink_wait_prev
sink_wait_same:
        lda $a2
        cmp sink_wait_prev
        beq sink_wait_same
        sta sink_wait_prev
        dec sink_delay
        bne sink_wait_tick
        rts

sink_scr_lo: .fill SINK_ROWS, <(INTRO_SCREEN_RAM + (SINK_TOP_ROW + i) * 40 + SINK_LEFT_COL)
sink_scr_hi: .fill SINK_ROWS, >(INTRO_SCREEN_RAM + (SINK_TOP_ROW + i) * 40 + SINK_LEFT_COL)
sink_col_lo: .fill SINK_ROWS, <($d800 + (SINK_TOP_ROW + i) * 40 + SINK_LEFT_COL)
sink_col_hi: .fill SINK_ROWS, >($d800 + (SINK_TOP_ROW + i) * 40 + SINK_LEFT_COL)
sink_steps: .byte 0
sink_delay: .byte 0
sink_wait_prev: .byte 0

// 2000 bytes of pure screen/colour data, relocated to the free $c000 block so it
// does not eat into the code segment (which must end before $4400).
// CPU-copied at runtime, so it needs no VIC visibility.
shooting_code_end:
* = $c000 "ShootingScreenData"

        .import source "shooting_screen_data.inc"

* = shooting_code_end "ShootingCodeResume"
