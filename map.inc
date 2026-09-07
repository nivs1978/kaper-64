.const MAP_BORDER_COLOR = 6

// Shared zero-page pointers (src_ptr/dst_ptr/len_lo/len_hi/bmp_char_ptr) now live in constants.inc.

// Normalize one-color cells by duplicating the nonzero nibble into the zero nibble.

draw_map:
        sei
        jsr hide_vic_screen
        jsr hide_ship

        lda #MAP_BORDER_COLOR
        sta $d020

        jsr decompress_map_bitmap
        jsr decompress_map_screen
        jsr normalize_map_screen_colours

        // Switch to hires bitmap mode only after the map is fully loaded.
        lda #$3b
        sta $d011

        lda #$08
        sta $d016

        lda $dd00
        and #$fc
        ora #VIC_BANK1_SELECT
        sta $dd00

        // Bitmap at $6000 and screen RAM at $4400 within VIC bank 1.
        lda #$18
        sta $d018

        jsr hide_ship
        jsr draw_map_status_table
        jsr show_vic_screen
        cli
        rts

game_loop:
game_loop_wait:
        jsr $ffe4
        beq game_loop_wait
        cmp #137
        beq game_toggle_sound
        cmp #17
        beq game_try_move_down
        cmp #145
        beq game_try_move_up
        cmp #29
        beq game_try_move_right
        cmp #157
        beq game_try_move_left
        jmp game_loop_wait

game_toggle_sound:
        jsr sound_toggle
        jmp game_loop_wait

game_try_move_up:
        jsr begin_pixel_move
        lda ship_y
        cmp #MAP_PIXEL_MIN_Y
        beq game_try_move_up_blocked
        bcc game_try_move_up_blocked
        sec
        sbc #MAP_GRID_STEP
        sta ship_y
        jmp resolve_pixel_move
game_try_move_up_blocked:
        jmp game_loop_wait

game_try_move_down:
        jsr begin_pixel_move
        lda ship_y
        cmp #MAP_PIXEL_MAX_Y
        bcs game_try_move_down_blocked
        clc
        adc #MAP_GRID_STEP
        sta ship_y
        jmp resolve_pixel_move
game_try_move_down_blocked:
        jmp game_loop_wait

game_try_move_left:
        jsr begin_pixel_move
        lda ship_x_hi
        cmp #>MAP_PIXEL_MIN_X
        bcc game_try_move_left_blocked
        bne game_try_move_left_apply
        lda ship_x
        cmp #<MAP_PIXEL_MIN_X
        beq game_try_move_left_blocked
        bcc game_try_move_left_blocked
game_try_move_left_apply:
        sec
        lda ship_x
        sbc #MAP_GRID_STEP
        sta ship_x
        lda ship_x_hi
        sbc #0
        sta ship_x_hi
        jmp resolve_pixel_move
game_try_move_left_blocked:
        jmp game_loop_wait

game_try_move_right:
        jsr begin_pixel_move
        lda ship_x_hi
        cmp #>MAP_PIXEL_MAX_X
        bcc game_try_move_right_apply
        bne game_try_move_right_blocked
        lda ship_x
        cmp #<MAP_PIXEL_MAX_X
        bcs game_try_move_right_blocked
game_try_move_right_apply:
        clc
        lda ship_x
        adc #MAP_GRID_STEP
        sta ship_x
        lda ship_x_hi
        adc #0
        sta ship_x_hi
        jmp resolve_pixel_move
game_try_move_right_blocked:
        jmp game_loop_wait

begin_pixel_move:
        lda ship_x
        sta ship_prev_x
        lda ship_x_hi
        sta ship_prev_x_hi
        lda ship_y
        sta ship_prev_y
        rts

resolve_pixel_move:
        inc voyage_turn_counter
        bne resolve_pixel_move_turn_incremented
        inc voyage_turn_counter+1
resolve_pixel_move_turn_incremented:
        jsr check_victory_conditions
        jsr apply_grain_upkeep
        jsr check_resource_limits
        jsr derive_map_cell_from_ship_position

        ldx map_ship_col
        ldy map_ship_row
        jsr get_map_cell_type
        sta current_map_cell_type

        cmp #MAP_CELL_LAND
        beq resolve_pixel_move_land
        cmp #MAP_CELL_SEA
        beq resolve_pixel_move_sea
        cmp #MAP_CELL_CLEARED
        beq resolve_pixel_move_cleared

        jsr position_ship
        lda current_map_cell_type
        jsr on_harbour_entered
        jmp game_loop_wait

resolve_pixel_move_cleared:
        jsr position_ship
        jmp resolve_pixel_move_no_event

resolve_pixel_move_sea:
        jsr position_ship
        // 25% random sea event chance, matching the BASIC game flow.
        lda $dc04
        and #$03
        beq resolve_pixel_move_random_event
resolve_pixel_move_no_event:
        jsr update_map_move_values
        jmp game_loop_wait
resolve_pixel_move_random_event:
        jsr on_random_sea_event
        jmp game_loop_wait

resolve_pixel_move_land:
        lda ship_prev_x
        sta ship_x
        lda ship_prev_x_hi
        sta ship_x_hi
        lda ship_prev_y
        sta ship_y
        jsr position_ship
        jsr on_land_collision
        jsr update_map_move_values
        jmp game_loop_wait

init_map_navigation:
        // Keep pixel coordinate as source of truth, then derive logical grid cell from it.
        lda #<MAP_START_PIXEL_X
        sta ship_x
        lda #>MAP_START_PIXEL_X
        sta ship_x_hi
        lda #MAP_START_PIXEL_Y
        sta ship_y
        lda #0
        sta voyage_turn_counter
        sta voyage_turn_counter+1
        sta grain_upkeep_remainder
        sta grain_upkeep_remainder+1
        sta ship_collision_counter
        lda #<200
        sta ship_repair_points
        lda #>200
        sta ship_repair_points+1
        lda #0
        sta last_harbour_id
        sta pending_prize_crew_lo
        sta pending_prize_crew_hi
        sta pending_prize_rigsdaler_lo
        sta pending_prize_rigsdaler_hi
        sta ship_sank_cause_lo
        sta ship_sank_cause_hi
        sta jewel_count
        sta jewel_count+1
        sta player_points
        sta player_points+1
        lda #<200
        sta crew_men
        lda #>200
        sta crew_men+1
        lda #<600
        sta treasury_rigsdaler
        lda #>600
        sta treasury_rigsdaler+1
        lda #<30
        sta grain_sacks
        lda #>30
        sta grain_sacks+1
        lda #<20
        sta ship_cannons
        lda #>20
        sta ship_cannons+1
        lda #<1000
        sta point_limit
        lda #>1000
        sta point_limit+1
        lda #<300
        sta turn_limit
        lda #>300
        sta turn_limit+1
        lda #1
        sta player_naval_title_index
        sta player_noble_title_index
        // BASIC line 89/102: GOSUB 102 runs once before the voyage starts, setting IDIF=2.
        lda #2
        sta difficulty_level
        lda #0
        lda #2
        sta encounter_count
        lda #1
        sta flee_count
        ldx #0
        lda #0
init_clear_map_cleared:
        sta map_cleared_row_masks,x
        inx
        cpx #60
        bne init_clear_map_cleared
        jsr derive_map_cell_from_ship_position
        jsr position_ship
        rts

sync_ship_to_grid_position:
        jsr derive_map_cell_from_ship_position
        jsr position_ship
        rts

derive_map_cell_from_ship_position:
        // map_ship_col = ((ship_x - MAP_GRID_ORIGIN_X) / MAP_GRID_STEP) + 1
        lda ship_x
        sec
        sbc #<MAP_GRID_ORIGIN_X
        sta map_rel_lo
        lda ship_x_hi
        sbc #>MAP_GRID_ORIGIN_X
        sta map_rel_hi
        lda #1
        sta map_ship_col
derive_col_loop:
        lda map_rel_hi
        bne derive_col_subtract
        lda map_rel_lo
        cmp #MAP_GRID_STEP
        bcc derive_col_done
derive_col_subtract:
        sec
        lda map_rel_lo
        sbc #MAP_GRID_STEP
        sta map_rel_lo
        lda map_rel_hi
        sbc #0
        sta map_rel_hi
        inc map_ship_col
        jmp derive_col_loop
derive_col_done:

        // map_ship_row = ((ship_y - MAP_GRID_ORIGIN_Y) / MAP_GRID_STEP) + 1
        lda ship_y
        sec
        sbc #MAP_GRID_ORIGIN_Y
        sta map_rel_lo
        lda #1
        sta map_ship_row
derive_row_loop:
        lda map_rel_lo
        cmp #MAP_GRID_STEP
        bcc derive_row_done
        sec
        sbc #MAP_GRID_STEP
        sta map_rel_lo
        inc map_ship_row
        jmp derive_row_loop
derive_row_done:
        rts

get_map_cell_type:
        // Input: X=column (1..29), Y=row (1..14)
        // Output: A=0 sea, 1 land, 2..8 harbour id.
        stx map_query_col
        sty map_query_row

        ldx #0
map_harbour_scan:
        cpx #MAP_HARBOUR_COUNT
        beq map_land_check
        lda map_harbour_col,x
        cmp map_query_col
        bne map_harbour_next
        lda map_harbour_row,x
        cmp map_query_row
        bne map_harbour_next
        lda map_harbour_id,x
        rts
map_harbour_next:
        inx
        bne map_harbour_scan

map_land_check:
        ldx map_query_col
        dex
        lda map_col_bit_mask,x
        sta map_query_bit_mask
        lda map_col_byte_index,x
        sta map_query_byte_index

        lda #<map_land_row_masks
        ldy #>map_land_row_masks
        jsr set_map_row_ptr
        ldy map_query_byte_index
        lda (src_ptr),y
        and map_query_bit_mask
        beq map_cell_check_cleared
        lda #MAP_CELL_LAND
        rts

map_cell_check_cleared:
        lda #<map_cleared_row_masks
        ldy #>map_cleared_row_masks
        jsr set_map_row_ptr
        ldy map_query_byte_index
        lda (src_ptr),y
        and map_query_bit_mask
        beq map_cell_is_sea
        lda #MAP_CELL_CLEARED
        rts

map_cell_is_sea:
        lda #MAP_CELL_SEA
        rts

// src_ptr = A/Y base + (map_query_row - 1) * 4 bytes per row.
set_map_row_ptr:
        sta src_ptr
        sty src_ptr+1
        ldy map_query_row
        dey
        tya
        asl
        asl
        clc
        adc src_ptr
        sta src_ptr
        bcc set_map_row_ptr_done
        inc src_ptr+1
set_map_row_ptr_done:
        rts

// BASIC 3000/3080: flag the tile the resolved encounter happened on.
mark_current_cell_cleared:
        lda map_ship_row
        sta map_query_row
        ldx map_ship_col
        dex
        lda map_col_bit_mask,x
        sta map_query_bit_mask
        lda map_col_byte_index,x
        sta map_query_byte_index
        lda #<map_cleared_row_masks
        ldy #>map_cleared_row_masks
        jsr set_map_row_ptr
        ldy map_query_byte_index
        lda (src_ptr),y
        ora map_query_bit_mask
        sta (src_ptr),y
        rts

on_land_collision:
        jsr sound_grounded
        inc ship_collision_counter
        sec
        lda ship_repair_points
        sbc #3
        sta ship_repair_points
        lda ship_repair_points+1
        sbc #0
        sta ship_repair_points+1
        bcs on_land_collision_done
        lda #0
        sta ship_repair_points
        sta ship_repair_points+1
on_land_collision_done:
        jsr check_ship_survival
        rts

on_random_sea_event:
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen

        // BASIC 1810 branch: I=1..10, where 1..8 are ships and 9..10 are fog events.
        jsr random_mod_10
        clc
        adc #1
        cmp #9
        bcs random_event_fog_branch
        sta enemy_index
        jsr run_enemy_encounter_text
        jmp random_event_done

random_event_fog_branch:
        jsr run_fog_event_text

random_event_done:
        jsr check_resource_limits
        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        rts

// Shared tail of both encounter intros: "<title> <name>," followed by the
// "now you must choose" line.
print_captain_and_choose:
        pstr(encounter_title_prefix_msg)
        jsr print_player_naval_title
        lda #' '
        jsr $ffd2
        jsr print_namebuf_zero_terminated
        lda #','
        jsr $ffd2
        jsr print_newline
        pstrnl(encounter_choose_msg)
        rts

run_enemy_encounter_text:
        inc encounter_count
        jsr init_enemy_from_index

        pstrnl(encounter_lookout_msg)
        pstrnl(encounter_ship_ahoy_msg)

        lda enemy_index
        cmp #8
        beq encounter_print_enemy_name
        pstr(encounter_english_msg)

encounter_print_enemy_name:
        lda enemy_name_ptr_lo
        sta src_ptr
        lda enemy_name_ptr_hi
        sta src_ptr+1
        jsr print_zero_terminated

        pstr(encounter_in_sight_msg)
        jsr print_double_newline

        jsr print_captain_and_choose

encounter_choice_1:
        jsr print_newline
        pstr(encounter_prompt_fight_flee)

encounter_choice_1_key:
        jsr read_key_blocking
        cmp #'A'
        beq encounter_attack_selected
        cmp #'a'
        beq encounter_attack_selected
        cmp #'F'
        beq encounter_flee_selected
        cmp #'f'
        beq encounter_flee_selected
        jmp encounter_choice_1_key

encounter_flee_selected:
        inc flee_count
        jsr sound_flee
        rts

encounter_attack_selected:
        jsr clear_text_notice_screen
        jsr print_newline
        pstrnl(encounter_prompt_fire_board_1)
        jsr print_newline
        pstrnl(encounter_prompt_fire_board_2)
        jsr print_newline
        jsr print_newline
        jsr print_newline
        jsr print_newline

        lda ship_cannons
        ora ship_cannons+1
        bne encounter_choice_2_key
        jmp encounter_board_selected

encounter_choice_2_key:
        jsr read_key_blocking
        cmp #'B'
        beq encounter_board_selected
        cmp #'b'
        beq encounter_board_selected
        cmp #'S'
        beq encounter_fire_selected
        cmp #'s'
        beq encounter_fire_selected
        jmp encounter_choice_2_key

encounter_board_selected:
        jsr run_boarding_fight_text
        lda battle_return_to_main_choice
        bne encounter_board_done
        jmp encounter_choice_1
encounter_board_done:
        rts

encounter_fire_selected:
        jsr run_shooting_minigame
        lda battle_return_to_main_choice
        beq encounter_fire_abort
        rts
encounter_fire_abort:
        jmp encounter_choice_1

run_fog_event_text:
        jsr sound_fog
        pstrnl(encounter_lookout_msg)
        pstrnl(fog_line_2)
        pstrnl(fog_line_3)
        pstrnl(fog_line_4)
        pstr(fog_line_5)
        jsr print_double_newline

        jsr print_captain_and_choose
        jsr print_newline

        pstr(fog_prompt_msg)

fog_prompt_key:
        jsr read_key_blocking
        cmp #'J'
        beq fog_investigate_yes
        cmp #'j'
        beq fog_investigate_yes
        cmp #'N'
        beq fog_investigate_no
        cmp #'n'
        beq fog_investigate_no
        jmp fog_prompt_key

fog_investigate_no:
        rts

fog_investigate_yes:
        jsr random_mod_10
        cmp #2
        bcc fog_outcome_enemy
        beq fog_outcome_treasure
fog_check_1:
        cmp #3
        bne fog_check_2
        jmp fog_outcome_jewel
fog_check_2:
        cmp #4
        bne fog_check_3
        jmp fog_outcome_cannon
fog_check_3:
        cmp #5
        bne fog_check_4
        jmp fog_outcome_crew
fog_check_4:
        cmp #8
        bne fog_check_5
        jmp fog_outcome_grain
fog_check_5:
        cmp #9
        bne fog_outcome_default
        jmp fog_outcome_plague
fog_outcome_default:

fog_outcome_nothing:
        jsr clear_text_notice_screen
        pstrnl(fog_nothing_line_1)
        pstrnl(fog_nothing_line_2)
        jsr show_press_any_key
        rts

fog_outcome_enemy:
        jsr clear_text_notice_screen
        pstrnl(fog_enemy_line_1)
        pstrnl(fog_enemy_line_2)
        jsr show_press_any_key
        jsr print_newline
        jsr random_enemy_roll
        sta enemy_index
        jsr run_enemy_encounter_text
        rts

fog_outcome_treasure:
        jsr clear_text_notice_screen
        pstrnl(fog_found_island_msg)
        pstrnl(fog_treasure_line_2)
        lda #<600
        sta damage_lo
        lda #>600
        sta damage_hi
        pstr(fog_treasure_line_3)
        pnum(damage_lo)
        pstrnl(fog_treasure_line_4)
        lda damage_lo
        clc
        adc treasury_rigsdaler
        sta treasury_rigsdaler
        lda damage_hi
        adc treasury_rigsdaler+1
        sta treasury_rigsdaler+1
        jsr show_press_any_key
        rts

fog_outcome_jewel:
        jsr clear_text_notice_screen
        jsr random_mod_6
        clc
        adc #2
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        clc
        adc jewel_count
        sta jewel_count
        lda damage_hi
        adc jewel_count+1
        sta jewel_count+1
fog_outcome_jewel_msg:
        pstrnl(fog_found_island_msg)
        pstr(fog_jewel_line_2)
        pnum(damage_lo)
        pstrnl(fog_jewel_line_3)
        jsr show_press_any_key
        rts

fog_outcome_cannon:
        jsr clear_text_notice_screen
        inc ship_cannons
        bne fog_outcome_cannon_msg
        inc ship_cannons+1
fog_outcome_cannon_msg:
        pstrnl(fog_found_island_msg)
        pstrnl(fog_cannon_line_2)
        jsr show_press_any_key
        rts

fog_outcome_crew:
        jsr clear_text_notice_screen
        jsr random_mod_40
        clc
        adc #9
        sta damage_lo
        lda #0
        sta damage_hi
        pstrnl(fog_found_island_msg)
        pstr(fog_jewel_line_2)
        pnum(damage_lo)
        pstrnl(fog_crew_line_3)
        lda damage_lo
        clc
        adc crew_men
        sta crew_men
        lda damage_hi
        adc crew_men+1
        sta crew_men+1
fog_outcome_crew_msg:
        jsr show_press_any_key
        rts

fog_outcome_grain:
        jsr clear_text_notice_screen
        jsr random_mod_6
        clc
        adc #2
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        clc
        adc grain_sacks
        sta grain_sacks
        lda damage_hi
        adc grain_sacks+1
        sta grain_sacks+1
fog_outcome_grain_msg:
        pstrnl(fog_found_island_msg)
        pstr(fog_jewel_line_2)
        pnum(damage_lo)
        pstrnl(fog_grain_line_3)
        jsr show_press_any_key
        rts

fog_outcome_plague:
        jsr clear_text_notice_screen
        jsr apply_plague_to_crew
        pstr(fog_found_island_msg)
        lda #' '
        jsr $ffd2
        pstrnl(fog_plague_line_1_suffix)
        pstrnl(fog_plague_line_2)
        pstrnl(fog_plague_line_3)
        jsr show_press_any_key
        rts

run_boarding_fight_text:
        lda #0
        sta battle_return_to_main_choice
        jsr clear_text_notice_screen
        jsr draw_fight_status_table
        jsr show_vic_screen
        jsr run_boarding_ship_animation

boarding_round:
        jsr apply_boarding_round
        jsr check_ship_survival
        jsr update_fight_status_values
        lda enemy_crew_hi
        bne boarding_continue
        lda enemy_crew_lo
        cmp #20
        bcs boarding_continue
        jmp boarding_enemy_low
boarding_continue:
        // Started here so a first-round win or loss is not preceded by a clipped tune.
        jsr sound_boarding
        jsr clear_boarding_text_area

        ldx #0
        ldy #6
        jsr set_screen_ptr_xy
        astr(boarding_battle_line_1)

        ldx #0
        ldy #7
        jsr set_screen_ptr_xy
        lda enemy_index
        cmp #8
        beq boarding_print_pirate_ship
        astr(boarding_enemy_line_prefix)
        lda enemy_name_ptr_lo
        sta src_ptr
        lda enemy_name_ptr_hi
        sta src_ptr+1
        jsr append_zero_terminated_screen
        astr(boarding_enemy_line_suffix)
        jmp boarding_ship_line_done
boarding_print_pirate_ship:
        astr(boarding_pirate_line)
boarding_ship_line_done:

        ldx #0
        ldy #8
        jsr set_screen_ptr_xy
        astr(boarding_enemy_losses_prefix)
        lda boarding_last_enemy_loss
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr append_word_decimal_screen
        astr(boarding_enemy_losses_suffix)

        ldx #0
        ldy #9
        jsr set_screen_ptr_xy
        astr(boarding_enemy_remaining_prefix)
        lda enemy_crew_lo
        sta num_print_lo
        lda enemy_crew_hi
        sta num_print_hi
        jsr append_word_decimal_screen
        astr(boarding_enemy_remaining_suffix)

        ldx #0
        ldy #10
        jsr set_screen_ptr_xy
        astr(boarding_player_losses_prefix)
        lda boarding_last_player_loss
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr append_word_decimal_screen
        astr(boarding_player_losses_suffix)

        ldx #0
        ldy #12
        jsr set_screen_ptr_xy
        astr(boarding_prompt_line_1)
        ldx #0
        ldy #13
        jsr set_screen_ptr_xy
        astr(boarding_prompt_line_2)

        jsr show_vic_screen

boarding_prompt_key:
        jsr read_key_blocking
        cmp #'K'
        bne boarding_prompt_key_lower_k
        jmp boarding_round
boarding_prompt_key_lower_k:
        cmp #'k'
        bne boarding_prompt_key_upper_t
        jmp boarding_round
boarding_prompt_key_upper_t:
        cmp #'T'
        beq boarding_back_to_ship
        cmp #'t'
        beq boarding_back_to_ship
        jmp boarding_prompt_key

boarding_back_to_ship:
        lda #0
        sta battle_return_to_main_choice
        rts

boarding_enemy_low:
        lda #7
        ldx #0
        ldy #6
        jsr show_surrender_flag
        jsr handle_enemy_surrender
        lda #1
        sta battle_return_to_main_choice
        rts

calculate_boarding_player_loss:
        jsr random_mod_64
        sta battle_random
        lda crew_men
        sta battle_product_lo
        lda crew_men+1
        sta battle_product_hi
        ldx difficulty_level
        jsr multiply_battle_product_by_x
        lda battle_random
        sta battle_factor
        jsr multiply_product_by_factor
        // BASIC 3140: MAND * ratio * (IDIF * RND / 30).
        // RND is approximated as a 0..63 fraction, hence divisor 1920.
        lda #<1920
        sta battle_divisor_lo
        lda #>1920
        sta battle_divisor_hi
        jsr divide_battle_product
        jsr scale_battle_loss_by_history
        lda battle_product_lo
        rts

calculate_boarding_enemy_loss:
        lda enemy_crew_lo
        sta battle_product_lo
        lda enemy_crew_hi
        sta battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        asl battle_product_lo
        rol battle_product_hi
        lda crew_men
        sta battle_divisor_lo
        lda crew_men+1
        sta battle_divisor_hi
        jsr divide_battle_product
        lda encounter_count
        clc
        adc flee_count
        sta battle_factor
        jsr multiply_product_by_factor
        lda encounter_count
        sta battle_divisor_lo
        lda #0
        sta battle_divisor_hi
        jsr divide_battle_product
        ldx difficulty_level
        jsr multiply_battle_product_by_x
        lda #10
        sta battle_divisor_lo
        lda #0
        sta battle_divisor_hi
        jsr divide_battle_product
        lda battle_product_lo
        cmp #154
        bcs cbel_iimm_max
        lda #154
        sta battle_product_lo
        lda #0
        sta battle_product_hi
cbel_iimm_max:
        lda battle_product_lo
        cmp #308
        bcc cbel_iimm_saved
        lda #307
        sta battle_product_lo
cbel_iimm_saved:
        lda battle_product_lo
        sta battle_iimm

        lda boarding_last_player_loss
        sta battle_product_lo
        lda #0
        sta battle_product_hi
        ldx #8
cbel_shift_loss:
        asl battle_product_lo
        rol battle_product_hi
        dex
        bne cbel_shift_loss
        lda battle_iimm
        sta battle_divisor_lo
        lda #0
        sta battle_divisor_hi
        jsr divide_battle_product
        lda enemy_crew_lo
        sta div_value_lo
        lda enemy_crew_hi
        sta div_value_hi
        jsr divide16_by10
        lda div_value_lo
        sta battle_base_lo
        lda battle_product_lo
        cmp battle_base_lo
        bcs cbel_loss_ready
        lda battle_base_lo
        sta battle_product_lo
cbel_loss_ready:
        lda battle_product_lo
        rts

multiply_product_by_factor:
        lda battle_product_lo
        sta battle_base_lo
        lda battle_product_hi
        sta battle_base_hi
        lda #0
        sta battle_product_lo
        sta battle_product_hi
        ldx battle_factor
multiply_factor_loop:
        cpx #0
        beq multiply_factor_done
        clc
        lda battle_product_lo
        adc battle_base_lo
        sta battle_product_lo
        lda battle_product_hi
        adc battle_base_hi
        sta battle_product_hi
        dex
        jmp multiply_factor_loop
multiply_factor_done:
        rts

multiply_battle_product_by_x:
        stx battle_factor
        jmp multiply_product_by_factor

divide_battle_product:
        lda #0
        sta battle_quotient_lo
        sta battle_quotient_hi
dbp_loop:
        lda battle_product_hi
        cmp battle_divisor_hi
        bcc dbp_done
        bne dbp_subtract
        lda battle_product_lo
        cmp battle_divisor_lo
        bcc dbp_done
dbp_subtract:
        sec
        lda battle_product_lo
        sbc battle_divisor_lo
        sta battle_product_lo
        lda battle_product_hi
        sbc battle_divisor_hi
        sta battle_product_hi
        inc battle_quotient_lo
        bne dbp_loop
        inc battle_quotient_hi
        jmp dbp_loop
dbp_done:
        lda battle_quotient_lo
        sta battle_product_lo
        lda battle_quotient_hi
        sta battle_product_hi
        rts

scale_battle_loss_by_history:
        lda encounter_count
        beq sbh_done
        clc
        adc flee_count
        sta battle_factor
        lda battle_product_lo
        sta battle_base_lo
        lda battle_product_hi
        sta battle_base_hi
        lda #0
        sta battle_product_lo
        sta battle_product_hi
        ldx battle_factor
sbh_multiply_loop:
        cpx #0
        beq sbh_divide
        clc
        lda battle_product_lo
        adc battle_base_lo
        sta battle_product_lo
        lda battle_product_hi
        adc battle_base_hi
        sta battle_product_hi
        dex
        jmp sbh_multiply_loop
sbh_divide:
        lda encounter_count
        sta battle_divisor_lo
        lda #0
        sta battle_divisor_hi
        jsr divide_battle_product
sbh_done:
        rts

apply_boarding_round:
        jsr calculate_boarding_player_loss
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        sta boarding_last_player_loss

        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        bcs boarding_player_crew_ok
        lda #0                      // clamp to 0 on underflow
        sta crew_men
        sta crew_men+1
        jmp boarding_player_done
boarding_player_crew_ok:
        sta crew_men+1
boarding_player_done:

        jsr calculate_boarding_enemy_loss
        sta damage_lo
        lda #0
        sta damage_hi
        lda damage_lo
        sta boarding_last_enemy_loss

        lda enemy_crew_lo
        sta enemy_crew_prev_lo
        lda enemy_crew_hi
        sta enemy_crew_prev_hi

        sec
        lda enemy_crew_lo
        sbc damage_lo
        sta enemy_crew_lo
        lda enemy_crew_hi
        sbc damage_hi
        bcs boarding_enemy_ok
        lda #0
        sta enemy_crew_lo
        sta enemy_crew_hi
boarding_enemy_ok:
        lda enemy_crew_hi
        cmp enemy_crew_prev_hi
        bcc boarding_enemy_monotonic_ok
        bne boarding_enemy_force_decrease
        lda enemy_crew_lo
        cmp enemy_crew_prev_lo
        bcc boarding_enemy_monotonic_ok
boarding_enemy_force_decrease:
        lda enemy_crew_prev_hi
        ora enemy_crew_prev_lo
        beq boarding_enemy_monotonic_ok
        sec
        lda enemy_crew_prev_lo
        sbc #1
        sta enemy_crew_lo
        lda enemy_crew_prev_hi
        sbc #0
        sta enemy_crew_hi
boarding_enemy_monotonic_ok:
        rts

handle_enemy_surrender:
        jsr mark_current_cell_cleared
        jsr clear_text_notice_screen
        pstrnl(surrender_line_1)

        // BASIC line 3010: IPOINT = IPOINT + INT(enemy rigsdaler / 10).
        // This reward is granted for every victory, before choosing sink or prize.
        lda enemy_rgsd_lo
        sta score_work_lo
        lda enemy_rgsd_hi
        sta score_work_hi
        lda #0
        sta score_award_lo
        sta score_award_hi
surrender_score_divide_loop:
        lda score_work_hi
        bne surrender_score_subtract
        lda score_work_lo
        cmp #10
        bcc surrender_score_divide_done
surrender_score_subtract:
        sec
        lda score_work_lo
        sbc #10
        sta score_work_lo
        lda score_work_hi
        sbc #0
        sta score_work_hi
        inc score_award_lo
        bne surrender_score_divide_loop
        inc score_award_hi
        jmp surrender_score_divide_loop
surrender_score_divide_done:
        clc
        lda player_points
        adc score_award_lo
        sta player_points
        lda player_points+1
        adc score_award_hi
        sta player_points+1

        clc
        lda treasury_rigsdaler
        adc enemy_rgsd_lo
        sta treasury_rigsdaler
        lda treasury_rigsdaler+1
        adc enemy_rgsd_hi
        sta treasury_rigsdaler+1

        clc
        lda grain_sacks
        adc enemy_grain
        sta grain_sacks
        bcc surrender_show_loot
        inc grain_sacks+1

surrender_show_loot:
        pstr(surrender_loot_rgsd_prefix)
        pnum(enemy_rgsd_lo)
        pstrnl(surrender_loot_rgsd_suffix)

        pstr(surrender_loot_rgsd_prefix)
        pnum(enemy_crew_lo)
        pstrnl(surrender_loot_men_suffix)

        pstr(surrender_loot_rgsd_prefix)
        lda enemy_grain
        sta num_print_lo
        lda #0
        sta num_print_hi
        jsr print_inline_word_decimal
        pstrnl(surrender_loot_grain_suffix)
        pstrnl(surrender_loot_take_line)
        jsr print_newline

        // Required prize crew = 5 + floor(enemy crew / 2), matching BASIC flow.
        lda enemy_crew_hi
        lsr
        sta damage_hi
        lda enemy_crew_lo
        ror
        sta damage_lo
        clc
        lda damage_lo
        adc #5
        sta damage_lo
        lda damage_hi
        adc #0
        sta damage_hi

        pstrnl(surrender_prize_need_line_1)
        pstr(surrender_prize_need_line_2_prefix)
        pnum(damage_lo)
        pstrnl(surrender_prize_need_line_2_suffix)
        pstr(surrender_prize_have_prefix)
        pnum(crew_men)
        pstrnl(surrender_prize_have_suffix)
        jsr print_newline

        pstrnl(surrender_line_2)
        pstr(surrender_line_3)
        jsr sound_surrender

surrender_prize_key:
        jsr read_key_blocking
        cmp #'S'
        beq surrender_sink
        cmp #'s'
        beq surrender_sink
        cmp #'P'
        beq surrender_prize
        cmp #'p'
        beq surrender_prize
        jmp surrender_prize_key

surrender_sink:
        clc
        lda crew_men
        adc enemy_crew_lo
        sta crew_men
        lda crew_men+1
        adc enemy_crew_hi
        sta crew_men+1
        // BASIC 3080: the ship holds 500 hands; anything over goes to the sharks.
        lda crew_men+1
        cmp #>500
        bcc surrender_sink_done
        bne surrender_sink_overflow
        lda crew_men
        cmp #<500
        bcc surrender_sink_done
        beq surrender_sink_done
surrender_sink_overflow:
        sec
        lda crew_men
        sbc #<500
        sta damage_lo
        lda crew_men+1
        sbc #>500
        sta damage_hi
        lda #<500
        sta crew_men
        lda #>500
        sta crew_men+1
        jsr print_newline
        pstr(sharks_prefix_msg)
        pnum(damage_lo)
        pstrnl(sharks_suffix_msg)
surrender_sink_done:
        rts

surrender_prize:
        sec
        lda crew_men
        sbc damage_lo
        sta crew_men
        lda crew_men+1
        sbc damage_hi
        sta crew_men+1

        // BASIC line 3019: IF IDIF<INT(RND*16) THEN the prize crew/gold reach K\benhavn safely.
        jsr random_mod_16
        cmp difficulty_level
        bcc surrender_prize_no_bonus
        beq surrender_prize_no_bonus
        clc
        lda pending_prize_crew_lo
        adc damage_lo
        sta pending_prize_crew_lo
        lda pending_prize_crew_hi
        adc damage_hi
        sta pending_prize_crew_hi
        clc
        lda pending_prize_rigsdaler_lo
        adc enemy_rgsd_lo
        sta pending_prize_rigsdaler_lo
        lda pending_prize_rigsdaler_hi
        adc enemy_rgsd_hi
        sta pending_prize_rigsdaler_hi
surrender_prize_no_bonus:

        jsr print_newline
        jsr print_newline
        pstr(surrender_prize_sailing_line)
        rts

init_enemy_from_index:
        sec
        sbc #1
        tax

        lda enemy_name_ptr_table_lo,x
        sta enemy_name_ptr_lo
        lda enemy_name_ptr_table_hi,x
        sta enemy_name_ptr_hi

        lda enemy_cannons_table,x
        sta enemy_cannons_lo
        lda #0
        sta enemy_cannons_hi

        lda enemy_crew_table_lo,x
        sta enemy_crew_lo
        lda enemy_crew_table_hi,x
        sta enemy_crew_hi

        lda enemy_rgsd_table_lo,x
        sta enemy_rgsd_lo
        lda enemy_rgsd_table_hi,x
        sta enemy_rgsd_hi

        lda enemy_grain_table,x
        sta enemy_grain

        jsr random_mod_50
        clc
        adc #100
        sta enemy_ship_hp_lo
        lda #0
        sta enemy_ship_hp_hi
        rts

random_enemy_roll:
        jsr random_mod_8
        clc
        adc #1
        rts

random_mod_10:
        lda $dc04
        and #$0f
        cmp #10
        bcc random_mod_10_done
        sec
        sbc #10
random_mod_10_done:
        rts

random_mod_16:
        lda $dc04
        and #$0f
        rts

random_mod_8:
        lda $dc05
        and #$07
        rts

random_mod_6:
        lda $dc05
        and #$07
        cmp #6
        bcs random_mod_6
        rts

random_mod_40:
        lda $dc04
        cmp #240
        bcs random_mod_40
random_mod_40_reduce:
        cmp #40
        bcc random_mod_40_done
        sec
        sbc #40
        jmp random_mod_40_reduce
random_mod_40_done:
        rts


random_mod_50:
        jsr random_mod_64
        cmp #50
        bcs random_mod_50
        rts

random_mod_64:
        lda $dc04
        and #$3f
        rts

clear_boarding_text_area:
        ldy #8
        sty fight_stat_row
clear_boarding_text_area_row:
        ldx #0
        ldy fight_stat_row
        jsr set_screen_ptr_xy
        ldy #0
        ldx #40
clear_boarding_text_area_col:
        lda #' '
        sta (dst_ptr),y
        iny
        dex
        bne clear_boarding_text_area_col
        inc fight_stat_row
        lda fight_stat_row
        cmp #15
        bcc clear_boarding_text_area_row
        rts

clear_text_notice_screen:
        jsr hide_vic_screen
        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2
        rts

print_player_naval_title:
        lda player_naval_title_index
        sec
        sbc #1
        tax
        lda naval_title_ptr_lo,x
        sta src_ptr
        lda naval_title_ptr_hi,x
        sta src_ptr+1
        jmp print_zero_terminated

print_player_noble_title:
        lda player_noble_title_index
        sec
        sbc #1
        tax
        lda noble_title_ptr_lo,x
        sta src_ptr
        lda noble_title_ptr_hi,x
        sta src_ptr+1
        jmp print_zero_terminated

print_namebuf_zero_terminated:
        ldy #0
print_namebuf_loop:
        lda namebuf,y
        beq print_namebuf_done
        jsr $ffd2
        iny
        bne print_namebuf_loop
print_namebuf_done:
        rts

apply_plague_to_crew:
        // BASIC-like outcome: roughly one third of crew is lost.
        lda crew_men
        sta crew_work_lo
        lda crew_men+1
        sta crew_work_hi
        lda #0
        sta crew_loss_lo
        sta crew_loss_hi

crew_loss_loop:
        lda crew_work_hi
        bne crew_loss_subtract
        lda crew_work_lo
        cmp #3
        bcc crew_loss_done
crew_loss_subtract:
        sec
        lda crew_work_lo
        sbc #3
        sta crew_work_lo
        lda crew_work_hi
        sbc #0
        sta crew_work_hi

        inc crew_loss_lo
        bne crew_loss_loop
        inc crew_loss_hi
        jmp crew_loss_loop

crew_loss_done:
        sec
        lda crew_men
        sbc crew_loss_lo
        sta crew_men
        lda crew_men+1
        sbc crew_loss_hi
        sta crew_men+1
        rts

show_press_any_key:
        pstr(intro_press_key)
        jsr show_vic_screen

show_press_any_key_wait:
        jsr $ffe4
        beq show_press_any_key_wait
        rts

// BASIC line 5000: die if crew_men <= 10 or ship_repair_points <= 20.
// Resets the stack and jumps to ship_sank if either threshold is breached.
check_ship_survival:
        lda crew_men+1
        bne check_survival_ok       // hi byte set → > 255, survives
        lda crew_men
        cmp #11
        bcc check_survival_sink     // crew <= 10 → sink
check_survival_ok:
        lda ship_repair_points+1
        bne check_survival_return   // hi byte set → > 255, survives
        lda ship_repair_points
        cmp #21
        bcc check_survival_sink     // repair <= 20 → sink
check_survival_return:
        rts
check_survival_sink:
        jmp ship_sank

// BASIC line 1175: consume floor(crew * difficulty / 800) grain each turn.
// If that consumes the remaining grain, lose ceil(crew / 10), matching
// INT(.9 * MAND) in the original game.
apply_grain_upkeep:
        lda #0
        sta damage_lo
        sta damage_hi
        lda grain_upkeep_remainder
        sta upkeep_old_rem_lo
        sta damage_lo
        lda grain_upkeep_remainder+1
        sta upkeep_old_rem_hi
        sta damage_hi
        lda #0
        sta damage_lo
        sta damage_hi
        ldx difficulty_level
apply_grain_product_loop:
        cpx #0
        beq apply_grain_product_done
        clc
        lda damage_lo
        adc crew_men
        sta damage_lo
        lda damage_hi
        adc crew_men+1
        sta damage_hi
        dex
        jmp apply_grain_product_loop

apply_grain_product_done:
        clc
        lda damage_lo
        adc upkeep_old_rem_lo
        sta damage_lo
        lda damage_hi
        adc upkeep_old_rem_hi
        sta damage_hi
        lda #0
        sta target_add_lo
        sta target_add_hi
apply_grain_divide_loop:
        lda damage_hi
        cmp #>800
        bcc apply_grain_divide_done
        bne apply_grain_divide_subtract
        lda damage_lo
        cmp #<800
        bcc apply_grain_divide_done
apply_grain_divide_subtract:
        sec
        lda damage_lo
        sbc #<800
        sta damage_lo
        lda damage_hi
        sbc #>800
        sta damage_hi
        inc target_add_lo
        bne apply_grain_divide_loop
        inc target_add_hi
        jmp apply_grain_divide_loop

apply_grain_divide_done:
        lda damage_lo
        sta grain_upkeep_remainder
        lda damage_hi
        sta grain_upkeep_remainder+1
        lda target_add_lo
        ora target_add_hi
        beq apply_grain_upkeep_done
        lda target_add_hi
        cmp grain_sacks+1
        bcc apply_grain_subtract
        bne apply_grain_depleted
        lda target_add_lo
        cmp grain_sacks
        bcc apply_grain_subtract
        lda grain_upkeep_remainder
        ora grain_upkeep_remainder+1
        beq apply_grain_subtract
apply_grain_depleted:
        lda #0
        sta grain_sacks
        sta grain_sacks+1
        jsr apply_tenth_crew_loss
        rts
apply_grain_subtract:
        sec
        lda grain_sacks
        sbc target_add_lo
        sta grain_sacks
        lda grain_sacks+1
        sbc target_add_hi
        sta grain_sacks+1
apply_grain_upkeep_done:
        rts

// Loss used by BASIC's MAND=INT(.9*MAND): ceil(MAND / 10) is removed.
apply_tenth_crew_loss:
        clc
        lda crew_men
        adc #9
        sta crew_work_lo
        lda crew_men+1
        adc #0
        sta crew_work_hi
        lda #0
        sta crew_loss_lo
        sta crew_loss_hi
apply_tenth_divide_loop:
        lda crew_work_hi
        bne apply_tenth_subtract
        lda crew_work_lo
        cmp #10
        bcc apply_tenth_divide_done
apply_tenth_subtract:
        sec
        lda crew_work_lo
        sbc #10
        sta crew_work_lo
        lda crew_work_hi
        sbc #0
        sta crew_work_hi
        inc crew_loss_lo
        bne apply_tenth_divide_loop
        inc crew_loss_hi
        jmp apply_tenth_divide_loop
apply_tenth_divide_done:
        sec
        lda crew_men
        sbc crew_loss_lo
        sta crew_men
        lda crew_men+1
        sbc crew_loss_hi
        sta crew_men+1
        jsr check_ship_survival
        rts

// BASIC lines 960-963: the ship cannot carry more than these amounts.
check_resource_limits:
        lda treasury_rigsdaler+1
        cmp #>30000
        bcc check_resource_crew
        bne resource_sink_treasury
        lda treasury_rigsdaler
        cmp #<30000
        bcc check_resource_crew
resource_sink_treasury:
        lda #<resource_treasury_msg
        sta ship_sank_cause_lo
        lda #>resource_treasury_msg
        sta ship_sank_cause_hi
        jmp resource_limit_sank

check_resource_crew:
        lda crew_men+1
        cmp #>500
        bcc check_resource_cannons
        bne resource_sink_crew
        lda crew_men
        cmp #<500
        bcc check_resource_cannons
resource_sink_crew:
        lda #<resource_crew_msg
        sta ship_sank_cause_lo
        lda #>resource_crew_msg
        sta ship_sank_cause_hi
        jmp resource_limit_sank

check_resource_cannons:
        lda ship_cannons+1
        cmp #>150
        bcc check_resource_grain
        bne resource_sink_cannons
        lda ship_cannons
        cmp #<150
        bcc check_resource_grain
resource_sink_cannons:
        lda #<resource_cannons_msg
        sta ship_sank_cause_lo
        lda #>resource_cannons_msg
        sta ship_sank_cause_hi
        jmp resource_limit_sank

check_resource_grain:
        lda grain_sacks+1
        cmp #>700
        bcc check_resource_limits_done
        bne resource_sink_grain
        lda grain_sacks
        cmp #<700
        bcc check_resource_limits_done
resource_sink_grain:
        lda #<resource_grain_msg
        sta ship_sank_cause_lo
        lda #>resource_grain_msg
        sta ship_sank_cause_hi
        jmp resource_limit_sank
check_resource_limits_done:
        rts

// BASIC lines 1170-1171: the point goal promotes you (1170 -> 6100), but the
// turn limit is a deadline - 1171 calls 6000, which ends with RETURN 5020 and
// so never comes back. This runs before hazards or upkeep.
check_victory_conditions:
        lda player_points+1
        cmp point_limit+1
        bcc check_victory_turn_limit
        bne player_won_stage
        lda player_points
        cmp point_limit
        bcs player_won_stage

check_victory_turn_limit:
        lda voyage_turn_counter+1
        cmp turn_limit+1
        bcc check_victory_not_yet
        bne player_out_of_turns
        lda voyage_turn_counter
        cmp turn_limit
        bcc check_victory_not_yet
player_out_of_turns:
        jmp voyage_deadline_lost
player_won_stage:
        jmp show_victory_and_advance
check_victory_not_yet:
        rts

// BASIC lines 6100-6119 and 102. Advance the difficulty, then calculate
// the next point and turn targets from the current score and voyage count.
show_victory_and_advance:
        jsr hide_ship
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        pstr(victory_msg)
        jsr print_double_newline
        pstrnl(victory_received_msg)
        pstr(victory_flaskepost_msg)
        jsr print_double_newline
        pstrnl(victory_komtesse_msg)

        inc difficulty_level
        inc difficulty_level
        lda player_naval_title_index
        cmp #5
        bcs victory_title_indices_done
        inc player_naval_title_index
        inc player_noble_title_index
victory_title_indices_done:

        // BASIC line 6102: IF IDIF<9 THEN 6113 (still climbing) ELSE 6103 (won it all).
        lda difficulty_level
        cmp #9
        bcc victory_still_climbing
        jmp victory_final_win

victory_still_climbing:
        pstrnl(victory_next_climb_msg)
        pstr(victory_next_goal_prefix)
        jsr print_noble_title_and_name
        jsr sound_promotion
        jsr show_press_any_key
        jmp victory_still_climbing_done

// BASIC lines 6103-6104: reaching admiral/kronprins ends the game in victory.
victory_final_win:
        pstrnl(victory_final_line_1)
        pstrnl(victory_final_line_2)
        jsr print_noble_title_and_name
        jsr sound_promotion
        jsr show_press_any_key
        jmp return_to_basic

// "<noble title> <name>!" plus newline - shared by both victory endings.
print_noble_title_and_name:
        jsr print_player_noble_title
        jsr print_namebuf_zero_terminated
        lda #'!'
        jsr $ffd2
        jmp print_newline

victory_still_climbing_done:
        // IPOINTLIM = IPOINT + 500 + 250 * IDIF.
        lda #<500
        sta target_add_lo
        lda #>500
        sta target_add_hi
        ldx difficulty_level
victory_point_add_loop:
        cpx #0
        beq victory_point_target
        clc
        lda target_add_lo
        adc #<250
        sta target_add_lo
        lda target_add_hi
        adc #>250
        sta target_add_hi
        dex
        bne victory_point_add_loop
victory_point_target:
        clc
        lda player_points
        adc target_add_lo
        sta point_limit
        lda player_points+1
        adc target_add_hi
        sta point_limit+1

        // ITURLIM = ITUR + 325 - 12.5 * IDIF; IDIF is always even.
        lda #<325
        sta target_add_lo
        lda #>325
        sta target_add_hi
        ldx difficulty_level
        txa
        lsr
        tax
victory_turn_subtract_loop:
        cpx #0
        beq victory_turn_target
        sec
        lda target_add_lo
        sbc #25
        sta target_add_lo
        lda target_add_hi
        sbc #0
        sta target_add_hi
        dex
        bne victory_turn_subtract_loop
victory_turn_target:
        clc
        lda voyage_turn_counter
        adc target_add_lo
        sta turn_limit
        lda voyage_turn_counter+1
        adc target_add_hi
        sta turn_limit+1

        jsr draw_map
        jsr load_ship_sprites
        jsr sync_ship_to_grid_position
        jsr show_ship
        jmp game_loop_wait

// Resets VIC/screen/charset/colours to the C64 power-on defaults, clears the
// screen, and warm-starts BASIC so the game ends with a normal READY. prompt.
return_to_basic:
        sei
        jsr restore_main_vic_bank
        lda #0
        sta $d015                   // hide all sprites
        lda #$1b
        sta $d011                   // default text mode, screen on
        lda #$c8
        sta $d016                   // default 40-column, no multicolor
        lda #14
        sta $d020                   // default light blue border
        lda #6
        sta $d021                   // default blue background
        lda #0
        sta $cc                     // enable cursor blink
        lda #$93
        jsr $ffd2                   // clear screen
        cli
        jmp $a474                   // BASIC warm start: prints READY. without wiping the loaded program

ship_sank:
        jsr hide_ship
        lda #6
        sta $d020
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        jsr sound_sink
        jmp end_game_summary

// BASIC line 6500: resource limits share the same tally screen; the cause line
// is printed from ship_sank_cause.
resource_limit_sank:
        jmp ship_sank

// BASIC line 1750: missing the harbour entrance on the last approach step sinks
// the ship outright - no points or final tally, unlike every other loss.
harbour_entrance_lost:
        jsr hide_ship
        lda #6
        sta $d020
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        jsr print_newline
        jsr print_double_newline
        pstrnl(ship_sank_msg)
        jsr sound_sink
        jmp end_game_restart

// BASIC 6000-6002: running out of turns loses the Komtesse, and you go over the
// side. 6000 ends with RETURN 5020, so it drops straight into the end summary.
voyage_deadline_lost:
        jsr hide_ship
        lda #6
        sta $d020
        jsr configure_intro_vic_bank
        jsr prepare_text_notice_screen
        pstrnl(deadline_sorrow_msg)
        pstrnl(deadline_komtesse_msg)
        pstrnl(deadline_wedding_msg)
        pstrnl(deadline_groom_msg)
        jsr print_newline
        pstrnl(deadline_overboard_msg)
        jsr sound_sink

end_game_summary:
        jsr print_newline
        jsr print_double_newline
        pstrnl(end_sank_msg)
        pstr(end_du_fik_msg)
        pnum(player_points)
        pstr(end_point_og_havde_msg)
        pnum(treasury_rigsdaler)
        pstrnl(end_rigsdaler_msg)
        lda ship_sank_cause_lo
        ora ship_sank_cause_hi
        beq end_game_no_cause
        lda ship_sank_cause_lo
        sta src_ptr
        lda ship_sank_cause_hi
        sta src_ptr+1
        jsr print_zero_terminated
        jsr print_newline
end_game_no_cause:
        jsr print_newline
        pstr(end_du_sluttede_msg)
        pnum(crew_men)
        pstr(end_mand_og_msg)
        pnum(ship_repair_points)
        pstrnl(end_rep_point_msg)
end_game_restart:
        lda #0
        sta $c6                     // drop keys buffered during the fight
        jsr show_press_any_key
        jsr audio_stop
        jsr restore_main_vic_bank
        ldx #$ff
        txs
        jmp restart_game

read_key_blocking:
        jsr show_vic_screen
        jsr $ffe4
        beq read_key_blocking
        cmp #137
        beq read_key_toggle_sound
        rts
read_key_toggle_sound:
        jsr sound_toggle
        jmp read_key_blocking

print_newline:
        lda #13
        jsr $ffd2
        rts

print_double_newline:
        jsr print_newline
        jsr print_newline
        rts

set_cursor_xy:
        // Callers pass X=column, Y=row.
        // KERNAL PLOT set-cursor expects C=0, X=row, Y=column.
        txa
        pha
        tya
        tax
        pla
        tay
        clc
        jsr $fff0
        rts

// ---------------------------------------------------------------
// Bitmap text rendering: draw PETSCII characters into the hires
// map bitmap (bank 1, $6000) using the charset at INTRO_CHARSET_RAM.
// ---------------------------------------------------------------

// Draw one PETSCII character at (bmp_char_col, bmp_char_row).
// A = PETSCII code.  Clobbers A, X, Y, bmp_char_ptr ($f7/$f8), dst_ptr, $f9.
draw_bmp_char:
        jsr petscii_to_screen_code
        // bmp_char_ptr = INTRO_CHARSET_RAM + screen_code * 8
        sta $f9
        lda #0
        asl $f9                     // shift 3× collecting carry into A
        rol
        asl $f9
        rol
        asl $f9
        rol
        clc
        adc #>INTRO_CHARSET_RAM
        sta bmp_char_ptr+1
        lda $f9
        sta bmp_char_ptr
        // dst_ptr = bmp_row_base[row] + col * 8
        ldx bmp_char_row
        lda bmp_row_base_lo, x
        sta dst_ptr
        lda bmp_row_base_hi, x
        sta dst_ptr+1
        lda bmp_char_col
        sta $f9
        lda #0
        asl $f9
        rol
        asl $f9
        rol
        asl $f9
        rol
        sta $fa                     // high byte of col*8 (use $fa to keep bmp_char_ptr intact)
        lda $f9
        clc
        adc dst_ptr
        sta dst_ptr
        lda $fa
        adc dst_ptr+1
        sta dst_ptr+1
        // Copy 8 pixel rows from charset to bitmap
        ldy #7
dbc_loop:
        lda (bmp_char_ptr), y
        sta (dst_ptr), y
        dey
        bpl dbc_loop
        // Set screen RAM color byte: white fg (1), black bg (0) = $10
        ldx bmp_char_row
        lda clr_row_base_lo, x
        sta dst_ptr
        lda clr_row_base_hi, x
        sta dst_ptr+1
        ldy bmp_char_col
        lda #$86                    // orange fg (8) on dark blue bg (6)
        sta (dst_ptr), y
        rts

// Print a zero-terminated PETSCII string into the bitmap.
// src_ptr = string address.  bmp_char_col/bmp_char_row must be set.
// Advances bmp_char_col for each character printed.
print_bmp_string:
        ldy #0
pbs_loop:
        lda (src_ptr), y
        beq pbs_done
        sty bmp_str_idx             // save string index across draw_bmp_char
        jsr draw_bmp_char
        inc bmp_char_col
        ldy bmp_str_idx
        iny
        bne pbs_loop
pbs_done:
        rts

// Render num_print_lo/hi as decimal digits at (bmp_char_col, bmp_char_row).
// bmp_char_col is advanced past the last digit.
print_bmp_decimal:
        lda num_print_lo
        ora num_print_hi
        beq pbd_zero
        lda #<bmp_decimal_buf
        sta dst_ptr
        lda #>bmp_decimal_buf
        sta dst_ptr+1
        jsr print_word_decimal
        ldy #0
pbd_digit_loop:
        cpy num_print_out_index
        beq pbd_done
        lda bmp_decimal_buf, y
        sty bmp_str_idx
        jsr draw_bmp_char
        inc bmp_char_col
        ldy bmp_str_idx
        iny
        bne pbd_digit_loop
        rts
pbd_zero:
        lda #<bmp_decimal_buf
        sta dst_ptr
        lda #>bmp_decimal_buf
        sta dst_ptr+1
        jsr print_word_decimal      // writes '0' to bmp_decimal_buf[0]
        lda bmp_decimal_buf
        jsr draw_bmp_char
        inc bmp_char_col
pbd_done:
        rts

// Clear a 6-char field with spaces then print decimal at col+1.
// X = start column, Y = row.  num_print_lo/hi = value.
print_bmp_stat_word:
        stx bmp_char_col
        sty bmp_char_row
        lda #6
        sta bmp_field_cnt
pbsw_clear:
        lda #' '
        jsr draw_bmp_char
        inc bmp_char_col
        dec bmp_field_cnt
        bne pbsw_clear
        // reset to col+1 (started at col, incremented 6 times, go back 5)
        lda bmp_char_col
        sec
        sbc #5
        sta bmp_char_col
        jsr print_bmp_decimal
        rts

// Draw label rows and current stat values into the map bitmap.
draw_map_status_table:
        // Row 21: three label cells, no pipe chars
        lda #1
        sta bmp_char_col
        lda #21
        sta bmp_char_row
        bstr(map_label_trk)
        lda #14
        sta bmp_char_col
        bstr(map_label_rgsdl)
        lda #27
        sta bmp_char_col
        bstr(map_label_korn)
        // Row 23: three label cells, no pipe chars
        lda #1
        sta bmp_char_col
        lda #23
        sta bmp_char_row
        bstr(map_label_maend)
        lda #14
        sta bmp_char_col
        bstr(map_label_skyts)
        lda #27
        sta bmp_char_col
        bstr(map_label_rep)
        jsr update_map_bmp_values
        // Row 0, col 2: POINT label + value (load only; points change via events)
        lda #2
        sta bmp_char_col
        lda #0
        sta bmp_char_row
        bstr(map_label_point)
        lda #' '                    // separator space before value
        jsr draw_bmp_char
        inc bmp_char_col
        lda player_points
        sta num_print_lo
        lda player_points+1
        sta num_print_hi
        jsr print_bmp_decimal
        lda #' '                    // one trailing space
        jsr draw_bmp_char
        // Row 0, col 16: goal display (M]L: x P] y TR[K)
        lda #16
        sta bmp_char_col
        bstr(map_goal_prefix)
        lda point_limit
        sta num_print_lo
        lda point_limit+1
        sta num_print_hi
        jsr print_bmp_decimal
        bstr(map_goal_mid)
        lda turn_limit
        sta num_print_lo
        lda turn_limit+1
        sta num_print_hi
        jsr print_bmp_decimal
        bstr(map_goal_suffix)
        rts

// Update only the two values that change during navigation.
update_map_move_values:
        ldx #6
        ldy #21
        bnum(voyage_turn_counter)
        ldx #33
        ldy #21
        bnum(grain_sacks)
        ldx #33
        ldy #23
        bnum(ship_repair_points)
        lda #' '                    // clear cell right of last digit in case REP shrank
        jsr draw_bmp_char
        rts

// Write current game stats as decimal into the map bitmap status area.
update_map_bmp_values:
        // TR[K (voyage_turn_counter)
        ldx #6
        ldy #21
        bnum(voyage_turn_counter)
        // RGSDL
        ldx #20
        ldy #21
        bnum(treasury_rigsdaler)
        // KORN
        ldx #33
        ldy #21
        bnum(grain_sacks)
        // M[ND
        ldx #6
        ldy #23
        bnum(crew_men)
        // SKYTS
        ldx #20
        ldy #23
        bnum(ship_cannons)
        // REP
        ldx #33
        ldy #23
        bnum(ship_repair_points)
        rts

draw_fight_status_table:
        ldx #0
        ldy #20
        wstr(fight_status_border_top)

        ldx #0
        ldy #21
        wstr(fight_status_row_1)

        ldx #0
        ldy #22
        wstr(fight_status_border_middle)

        ldx #0
        ldy #23
        wstr(fight_status_row_2)

        ldx #0
        ldy #24
        wstr(fight_status_border_bottom)

        jsr update_fight_status_values
        rts

update_fight_status_values:
        // TR[ K
        ldx #7
        ldy #21
        fnum(voyage_turn_counter)

        // RGSDL
        ldx #20
        ldy #21
        fnum(treasury_rigsdaler)

        // KORN
        ldx #33
        ldy #21
        fnum(grain_sacks)

        // SKYTS
        ldx #20
        ldy #23
        fnum(ship_cannons)

        // REP
        ldx #31
        ldy #23
        fnum(ship_repair_points)

        // M[ND
        ldx #7
        ldy #23
        fnum(crew_men)
        rts

print_fight_byte_stat:
        sta num_print_lo
        lda #0
        sta num_print_hi
        jmp print_fight_word_stat

print_fight_word_stat:
        sty fight_stat_row
        jsr set_screen_ptr_xy

        ldy #0
        lda #' '
clear_fight_value_field_loop:
        sta (dst_ptr),y
        iny
        cpy #6
        bne clear_fight_value_field_loop

        inc dst_ptr
        bne print_fight_word_stat_ptr_ok
        inc dst_ptr+1
print_fight_word_stat_ptr_ok:
        jsr print_word_decimal
        rts

set_screen_ptr_xy:
        lda screen_row_addr_lo,y
        sta dst_ptr
        lda screen_row_addr_hi,y
        sta dst_ptr+1
        txa
        clc
        adc dst_ptr
        sta dst_ptr
        bcc set_screen_ptr_xy_done
        inc dst_ptr+1
set_screen_ptr_xy_done:
        rts

write_zero_terminated_screen:
        jsr set_screen_ptr_xy
        jmp append_zero_terminated_screen

append_zero_terminated_screen:
        ldy #0
append_zero_terminated_screen_loop:
        lda (src_ptr),y
        beq append_zero_terminated_screen_done
        jsr petscii_to_screen_code
        sta (dst_ptr),y
        iny
        bne append_zero_terminated_screen_loop
append_zero_terminated_screen_done:
        tya
        clc
        adc dst_ptr
        sta dst_ptr
        bcc append_zero_terminated_screen_ptr_ok
        inc dst_ptr+1
append_zero_terminated_screen_ptr_ok:
        rts

append_word_decimal_screen:
        lda num_print_lo
        ora num_print_hi
        beq append_word_decimal_screen_zero
        jsr print_word_decimal
        lda num_print_out_index
        jmp append_word_decimal_screen_advance
append_word_decimal_screen_zero:
        jsr print_word_decimal
        lda #1
append_word_decimal_screen_advance:
        clc
        adc dst_ptr
        sta dst_ptr
        bcc append_word_decimal_screen_ptr_ok
        inc dst_ptr+1
append_word_decimal_screen_ptr_ok:
        rts

petscii_to_screen_code:
        // Uppercase/graphics conversion for direct screen RAM writes.
        // $40-$5F  -> $00-$1F
        // $60-$7F  -> $40-$5F
        // $A0-$BF  -> $60-$7F
        cmp #$40
        bcc petscii_to_screen_done
        cmp #$60
        bcc petscii_to_screen_sub_64
        cmp #$80
        bcc petscii_to_screen_sub_32
        cmp #$a0
        bcc petscii_to_screen_done
        cmp #$c0
        bcs petscii_to_screen_done
petscii_to_screen_sub_64:
        sec
        sbc #$40
        jmp petscii_to_screen_done
petscii_to_screen_sub_32:
        sec
        sbc #$20
petscii_to_screen_done:
        rts

print_word_decimal:
        lda num_print_lo
        ora num_print_hi
        bne print_word_decimal_begin
        ldy #0
        lda #'0'
        sta (dst_ptr),y
        rts

print_word_decimal_begin:
        lda #0
        sta num_print_started
        sta num_print_out_index
        ldx #0

print_word_decimal_digit:
        ldy #0
print_word_decimal_subtract_check:
        lda num_print_hi
        cmp decimal_divisor_hi,x
        bcc print_word_decimal_digit_ready
        bne print_word_decimal_subtract
        lda num_print_lo
        cmp decimal_divisor_lo,x
        bcc print_word_decimal_digit_ready
print_word_decimal_subtract:
        sec
        lda num_print_lo
        sbc decimal_divisor_lo,x
        sta num_print_lo
        lda num_print_hi
        sbc decimal_divisor_hi,x
        sta num_print_hi
        iny
        bne print_word_decimal_subtract_check

print_word_decimal_digit_ready:
        tya
        bne print_word_decimal_emit
        lda num_print_started
        bne print_word_decimal_emit
        cpx #4
        bne print_word_decimal_next

print_word_decimal_emit:
        tya
        clc
        adc #'0'
        ldy num_print_out_index
        sta (dst_ptr),y
        inc num_print_out_index
        lda #1
        sta num_print_started

print_word_decimal_next:
        inx
        cpx #5
        bne print_word_decimal_digit
        rts

print_inline_word_decimal:
        lda #' '
        jsr $ffd2
        ldx #0
print_inline_word_decimal_clear:
        lda #0
        sta inline_decimal_buffer,x
        inx
        cpx #6
        bne print_inline_word_decimal_clear
        lda #<inline_decimal_buffer
        sta dst_ptr
        lda #>inline_decimal_buffer
        sta dst_ptr+1
        jsr print_word_decimal
        lda #<inline_decimal_buffer
        sta src_ptr
        lda #>inline_decimal_buffer
        sta src_ptr+1
        jsr print_zero_terminated
        lda #' '
        jsr $ffd2
        rts

// on_harbour_entered lives in harbour.inc: it shows the trading menu (buy/sell)
// and, for K\benhavn, pays out any pending prize-ship crew/rigsdaler.

prepare_text_notice_screen:
        // Ensure text mode while showing event/harbour notices.
        jsr hide_vic_screen
        lda $d011
        and #$df
        sta $d011
        lda $d016
        and #$ef
        sta $d016

        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2
        rts

print_zero_terminated:
        ldy #0
print_zero_terminated_loop:
        lda (src_ptr),y
        beq print_zero_terminated_done
        jsr $ffd2
        iny
        bne print_zero_terminated_loop
print_zero_terminated_done:
        rts

// Inline-argument string printers - see the pstr/pstrnl macros in constants.inc.
// The string address sits in the two bytes right after the JSR; fetch_inline_str_arg
// reads it via the stacked return address and skips past it before returning.
print_str_inline:
        jsr fetch_inline_str_arg
        jmp print_zero_terminated

print_str_inline_nl:
        jsr fetch_inline_str_arg
        jsr print_zero_terminated
        jmp print_newline

// Same idea for the very common "copy a 16-bit variable into num_print_lo/hi and
// print it" sequence - 5 bytes per call site instead of 13.
print_var_decimal:
        jsr fetch_inline_str_arg
        jsr load_num_print_from_src_ptr
        jmp print_inline_word_decimal

print_bmp_stat_var:
        jsr fetch_inline_str_arg
        jsr load_num_print_from_src_ptr
        jmp print_bmp_stat_word

print_fight_stat_var:
        jsr fetch_inline_str_arg
        jsr load_num_print_from_src_ptr
        jmp print_fight_word_stat

// Note: every inline-argument entry point must call fetch_inline_str_arg directly -
// it reads the caller's return address at a fixed stack depth.
load_num_print_from_src_ptr:
        sty inline_saved_y
        lda src_ptr
        cmp #<grain_sacks
        bne load_num_print_regular
        lda src_ptr+1
        cmp #>grain_sacks
        bne load_num_print_regular
        jsr calculate_grain_display_value
        lda grain_display_value
        sta num_print_lo
        lda grain_display_value+1
        sta num_print_hi
        ldy inline_saved_y
        rts
load_num_print_regular:
        ldy #0
        lda (src_ptr),y
        sta num_print_lo
        iny
        lda (src_ptr),y
        sta num_print_hi
        ldy inline_saved_y
        rts

// Render remaining grain rounded to the nearest whole sack. The stored whole
// value is the pre-fraction amount; the remainder is fractional grain already
// consumed, so it must be subtracted for display.
calculate_grain_display_value:
        lda grain_sacks
        sta grain_display_value
        lda grain_sacks+1
        sta grain_display_value+1
        lda grain_upkeep_remainder+1
        cmp #>400
        bcc calculate_grain_display_done
        bne calculate_grain_display_round_down
        lda grain_upkeep_remainder
        cmp #<400
        bcc calculate_grain_display_done
calculate_grain_display_round_down:
        lda grain_display_value
        bne calculate_grain_display_decrement_low
        dec grain_display_value+1
calculate_grain_display_decrement_low:
        dec grain_display_value
calculate_grain_display_done:
        rts

append_str_inline:
        jsr fetch_inline_str_arg
        jmp append_zero_terminated_screen

write_str_inline:
        jsr fetch_inline_str_arg
        jmp write_zero_terminated_screen

print_bmp_str_inline:
        jsr fetch_inline_str_arg
        jmp print_bmp_string

fetch_inline_str_arg:
        // X/Y must survive: several inline-argument targets take X=column, Y=row.
        // Stash them in zero page rather than on the stack, which tsx has to index.
        stx inline_saved_x
        sty inline_saved_y
        // Stack from $0101,x: own return address, then the caller's (= inline arg - 1).
        tsx
        lda $0103,x
        sta inline_arg_ptr
        lda $0104,x
        sta inline_arg_ptr+1
        ldy #1
        lda (inline_arg_ptr),y
        sta src_ptr
        iny
        lda (inline_arg_ptr),y
        sta src_ptr+1
        lda inline_arg_ptr
        clc
        adc #2
        sta $0103,x
        lda inline_arg_ptr+1
        adc #0
        sta $0104,x
        ldx inline_saved_x
        ldy inline_saved_y
        rts

copy_len_bytes:
        ldy #0
copy_len_bytes_loop:
        lda (src_ptr),y
        sta (dst_ptr),y
        inc src_ptr
        bne copy_len_bytes_src_ok
        inc src_ptr+1
copy_len_bytes_src_ok:
        inc dst_ptr
        bne copy_len_bytes_dst_ok
        inc dst_ptr+1
copy_len_bytes_dst_ok:
        lda len_lo
        bne copy_len_bytes_dec_lo
        dec len_hi
copy_len_bytes_dec_lo:
        dec len_lo
        lda len_hi
        ora len_lo
        bne copy_len_bytes_loop
        rts

// One-colour cells: duplicate the nonzero nibble into the zero one, in place
// over the unpacked screen colour data.
normalize_map_screen_colours:
        lda #<INTRO_SCREEN_RAM
        sta dst_ptr
        lda #>INTRO_SCREEN_RAM
        sta dst_ptr+1
normalize_map_cell:
        ldy #0
        lda (dst_ptr),y
        sta screen_tmp
        and #$0f
        bne normalize_check_high_zero
        lda screen_tmp
        lsr
        lsr
        lsr
        lsr
        ora screen_tmp
        sta screen_tmp
        jmp normalize_store

normalize_check_high_zero:
        lda screen_tmp
        and #$f0
        bne normalize_store
        lda screen_tmp
        asl
        asl
        asl
        asl
        ora screen_tmp
        sta screen_tmp

normalize_store:
        lda screen_tmp
        ldy #0
        sta (dst_ptr),y
        inc dst_ptr
        bne normalize_check_end
        inc dst_ptr+1
normalize_check_end:
        lda dst_ptr+1
        cmp #>MAP_SCREEN_END
        bne normalize_map_cell
        lda dst_ptr
        cmp #<MAP_SCREEN_END
        bne normalize_map_cell
        rts

screen_tmp: .byte 0

// Logical map navigation state (map-local).
map_ship_col: .byte MAP_START_COL
map_ship_row: .byte MAP_START_ROW
ship_prev_x: .byte 0
ship_prev_x_hi: .byte 0
ship_prev_y: .byte 0
map_query_col: .byte 0
map_query_row: .byte 0
map_query_bit_mask: .byte 0
map_query_byte_index: .byte 0
current_map_cell_type: .byte 0
last_harbour_id: .byte 0
ship_collision_counter: .byte 0
difficulty_level: .byte 0
encounter_count: .byte 0
flee_count: .byte 0
target_add_lo: .byte 0
target_add_hi: .byte 0
score_work_lo: .byte 0
score_work_hi: .byte 0
score_award_lo: .byte 0
score_award_hi: .byte 0
crew_work_lo: .byte 0
crew_work_hi: .byte 0
crew_loss_lo: .byte 0
crew_loss_hi: .byte 0
enemy_index: .byte 1
enemy_name_ptr_lo: .byte 0
enemy_name_ptr_hi: .byte 0
enemy_cannons_lo: .byte 0
enemy_cannons_hi: .byte 0
enemy_crew_lo: .byte 0
enemy_crew_hi: .byte 0
enemy_crew_prev_lo: .byte 0
enemy_crew_prev_hi: .byte 0
enemy_rgsd_lo: .byte 0
enemy_rgsd_hi: .byte 0
enemy_grain: .byte 0
enemy_ship_hp_lo: .byte 0
enemy_ship_hp_hi: .byte 0
boarding_last_player_loss: .byte 0
boarding_last_enemy_loss: .byte 0
battle_return_to_main_choice: .byte 0
damage_lo: .byte 0
damage_hi: .byte 0
battle_random: .byte 0
battle_factor: .byte 0
battle_base_lo: .byte 0
battle_base_hi: .byte 0
battle_product_lo: .byte 0
battle_product_hi: .byte 0
battle_quotient_lo: .byte 0
battle_quotient_hi: .byte 0
battle_divisor_lo: .byte 0
battle_divisor_hi: .byte 0
battle_iimm: .byte 0
grain_upkeep_remainder: .word 0
upkeep_old_rem_lo: .byte 0
upkeep_old_rem_hi: .byte 0
grain_display_value: .word 0
ship_sank_cause_lo: .byte 0
ship_sank_cause_hi: .byte 0
num_print_lo: .byte 0
num_print_hi: .byte 0
num_print_started: .byte 0
num_print_out_index: .byte 0
inline_decimal_buffer: .fill 6, 0
fight_stat_row: .byte 0
bmp_char_col:   .byte 0
bmp_char_row:   .byte 0
bmp_str_idx:    .byte 0
bmp_field_cnt:  .byte 0
bmp_decimal_buf:.fill 6, 0
player_naval_title_index: .byte 1
player_noble_title_index: .byte 1

// Temporary state for pixel->cell conversion.
map_rel_lo: .byte 0
map_rel_hi: .byte 0

map_harbour_col: .byte 25,25,18,9,7,11,24
map_harbour_row: .byte 12,7,8,1,4,13,3
map_harbour_id:  .byte 2,3,4,5,6,7,8

map_harbour_name_ptr_lo:
        .byte <harbour_copenhagen_msg
        .byte <harbour_helsingor_msg
        .byte <harbour_hundested_msg
        .byte <harbour_grenaa_msg
        .byte <harbour_ebeltoft_msg
        .byte <harbour_kalundborg_msg
        .byte <harbour_molle_msg
map_harbour_name_ptr_hi:
        .byte >harbour_copenhagen_msg
        .byte >harbour_helsingor_msg
        .byte >harbour_hundested_msg
        .byte >harbour_grenaa_msg
        .byte >harbour_ebeltoft_msg
        .byte >harbour_kalundborg_msg
        .byte >harbour_molle_msg

// For each column 1..29, select which of 4 row-mask bytes to test and which bit.
map_col_byte_index:
        .byte 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3
map_col_bit_mask:
        .byte $01,$02,$04,$08,$10,$20,$40,$80,$01,$02,$04,$08,$10,$20,$40,$80
        .byte $01,$02,$04,$08,$10,$20,$40,$80,$01,$02,$04,$08,$10

// Land mask rows from original DOS map data (1 bit per column, 29 columns/row).
map_land_row_masks:
        .byte $80,$00,$00,$0C
        .byte $D8,$00,$00,$0C
        .byte $76,$00,$00,$09
        .byte $52,$00,$00,$0E
        .byte $4A,$00,$00,$06
        .byte $02,$00,$70,$04
        .byte $02,$00,$F8,$0C
        .byte $32,$30,$F0,$08
        .byte $23,$C4,$F4,$1A
        .byte $20,$C0,$F4,$10
        .byte $20,$60,$F4,$10
        .byte $04,$FE,$F7,$01
        .byte $00,$98,$FF,$01
        .byte $00,$04,$F0,$01
        .byte $00,$00,$F0,$00

// One bit per column, same 4-byte row stride as map_land_row_masks. Set by
// mark_current_cell_cleared, wiped at the start of every voyage.
map_cleared_row_masks:
        .fill 60, 0

// BASIC 1990-2020 and 2950-2990: raise the pole, then strike the enemy colours.
// The blue field of the union jack is the screen background showing through.
// A = column, X = top row, Y = pole height in rows. Drawn over the current screen.
show_surrender_flag:
        sta flag_pole_col
        stx flag_pole_top_row
        sty flag_pole_rows
        jsr draw_flag_pole
        jsr load_flag_sprites
        jsr set_sprite_pointers_intro

        lda #0
        sta $d017
        sta $d01c
        sta $d01b
        lda flag_sprite_mask
        sta $d01d               // double width so the flag is not square
        sta $d015

        jsr set_flag_sprite_x

        lda flag_pole_top_row
        asl
        asl
        asl
        clc
        adc #50
        sta flag_y
        lda flag_pole_top_row
        clc
        adc flag_pole_rows
        sec
        sbc #3
        asl
        asl
        asl
        clc
        adc #50
        sta flag_bottom_y

flag_anim_loop:
        lda flag_y
        sta $d001
        sta $d003
        ldx #2
        jsr flag_wait_frames
        lda flag_y
        clc
        adc #2
        sta flag_y
        cmp flag_bottom_y
        bcc flag_anim_loop

        ldx #40
        jsr flag_wait_frames
        lda #0
        sta $d015
        sta $d01d
        rts

// Sprite X is 24 + (column + 1) * 8, which needs the MSB past column 27.
set_flag_sprite_x:
        lda #0
        sta flag_x_hi
        lda flag_pole_col
        asl
        bcc flag_x_b1
        inc flag_x_hi
flag_x_b1:
        asl
        bcc flag_x_b2
        inc flag_x_hi
flag_x_b2:
        asl
        bcc flag_x_b3
        inc flag_x_hi
flag_x_b3:
        clc
        adc #32
        sta $d000
        sta $d002
        bcc flag_x_done
        inc flag_x_hi
flag_x_done:
        lda flag_x_hi
        beq flag_x_low
        lda flag_sprite_mask
        sta $d010
        rts
flag_x_low:
        lda #0
        sta $d010
        rts

draw_flag_pole:
        lda flag_pole_top_row
        sta flag_row_index
        lda flag_pole_rows
        sta flag_rows_left
        lda #$51                // filled circle forms the truck of the pole
        sta flag_pole_char
flag_pole_row:
        ldx flag_pole_col
        ldy flag_row_index
        jsr set_screen_ptr_xy
        ldy #0
        lda flag_pole_char
        sta (dst_ptr),y
        lda dst_ptr+1
        clc
        adc #$94                // screen $4400 -> colour $d800
        sta dst_ptr+1
        lda #1
        sta (dst_ptr),y
        lda #$5d                // vertical bar for the rest of the pole
        sta flag_pole_char
        inc flag_row_index
        dec flag_rows_left
        bne flag_pole_row
        rts

load_flag_sprites:
        lda enemy_index
        cmp #8
        beq load_pirate_flag
        lda #<flag_gb_red_sprite
        ldx #>flag_gb_red_sprite
        ldy #0
        jsr copy_flag_sprite
        lda #<flag_gb_white_sprite
        ldx #>flag_gb_white_sprite
        ldy #1
        jsr copy_flag_sprite
        lda #2
        sta $d027               // red cross sits on the higher-priority sprite
        lda #1
        sta $d028
        lda #%00000011
        sta flag_sprite_mask
        rts
load_pirate_flag:
        lda #<flag_pirate_sprite
        ldx #>flag_pirate_sprite
        ldy #0
        jsr copy_flag_sprite
        lda #1
        sta $d027
        lda #%00000001
        sta flag_sprite_mask
        rts

// A/X = sprite source, Y = slot 0 or 1 in the bank 1 sprite area.
copy_flag_sprite:
        sta src_ptr
        stx src_ptr+1
        lda #<SPRITE_DATA_BANK1
        sta dst_ptr
        lda #>SPRITE_DATA_BANK1
        sta dst_ptr+1
        cpy #0
        beq copy_flag_bytes
        clc
        lda dst_ptr
        adc #64
        sta dst_ptr
        bcc copy_flag_bytes
        inc dst_ptr+1
copy_flag_bytes:
        ldy #62
copy_flag_loop:
        lda (src_ptr),y
        sta (dst_ptr),y
        dey
        bpl copy_flag_loop
        rts

flag_wait_frames:
        lda $a2
        sta flag_wait_prev
flag_wait_same:
        lda $a2
        cmp flag_wait_prev
        beq flag_wait_same
        sta flag_wait_prev
        dex
        bne flag_wait_frames
        rts

flag_pole_char: .byte 0
flag_sprite_mask: .byte 0
flag_y: .byte 0
flag_bottom_y: .byte 0
flag_pole_col: .byte 0
flag_pole_top_row: .byte 0
flag_pole_rows: .byte 0
flag_row_index: .byte 0
flag_rows_left: .byte 0
flag_wait_prev: .byte 0
flag_x_hi: .byte 0

code_segment_end:
* = $8a00 "FlagSpriteData"
flag_gb_red_sprite:
        .byte $00,$00,$00,$20,$18,$04,$10,$18
        .byte $08,$08,$18,$10,$04,$18,$20,$02
        .byte $18,$40,$01,$18,$80,$00,$99,$00
        .byte $00,$5a,$00,$00,$18,$00,$7f,$ff
        .byte $fe,$7f,$ff,$fe,$00,$18,$00,$00
        .byte $5a,$00,$00,$99,$00,$01,$18,$80
        .byte $02,$18,$40,$04,$18,$20,$08,$18
        .byte $10,$10,$18,$08,$00,$00,$00,$04

flag_gb_white_sprite:
        .byte $ff,$ff,$ff,$d8,$00,$0b,$ac,$00
        .byte $17,$96,$00,$2d,$8b,$00,$59,$85
        .byte $80,$b1,$82,$c1,$61,$81,$62,$c1
        .byte $80,$a5,$81,$ff,$e7,$ff,$80,$00
        .byte $01,$80,$00,$01,$ff,$e7,$ff,$81
        .byte $a5,$01,$83,$66,$81,$86,$83,$41
        .byte $8d,$01,$a1,$9a,$00,$d1,$b4,$00
        .byte $69,$e8,$00,$35,$ff,$ff,$ff,$01

flag_pirate_sprite:
        .byte $ff,$ff,$ff,$80,$00,$01,$80,$7e
        .byte $01,$80,$ff,$01,$81,$ff,$81,$81
        .byte $99,$81,$81,$99,$81,$81,$ff,$81
        .byte $81,$e7,$81,$80,$7e,$01,$90,$24
        .byte $09,$b8,$3c,$1d,$98,$3c,$19,$86
        .byte $00,$61,$81,$e7,$81,$80,$18,$01
        .byte $80,$e7,$01,$97,$00,$e9,$b8,$00
        .byte $1d,$90,$00,$09,$ff,$ff,$ff,$0f

// Program code ends before $4400 (VIC bank 1 screen RAM, shared
// with the map's bitmap-mode colour screen); relocate the remaining pure text
// data into the unused gap after the shooting sprites ($5140-$5fff) and before
// the map bitmap ($6000) so it never grows into that screen memory again.
* = $5140 "HarbourAndMapText"
encounter_lookout_msg:  .text "UDKIGGEN MELDER:"
                        .byte 0
encounter_ship_ahoy_msg:.text "SHIP OH\J!"
                        .byte 0
encounter_english_msg:  .text "ENGELSK "
                        .byte 0
encounter_in_sight_msg: .text " I SIGTE"
                        .byte 0
encounter_title_prefix_msg:
                        .text "N] "
                        .byte 0
encounter_choose_msg:   .text "NU M] DU V[LGE:"
                        .byte 0
encounter_prompt_fight_flee:
                        .text "VIL DU ANGRIBE (A) ELLER FLYGTE (F)?"
                        .byte 0
encounter_prompt_fire_board_1:
                        .text "VIL DU BORDE HENDE (B) ELLER"
                        .byte 0
encounter_prompt_fire_board_2:
                        .text "VIL DU SKYDE (S) P] HENDE?"
                        .byte 0
fight_status_border_top:
                        .byte 176
                        .fill 12,96
                        .byte 178
                        .fill 12,96
                        .byte 178
                        .fill 12,96
                        .byte 174,0
fight_status_border_middle:
                        .byte 171
                        .fill 12,96
                        .byte 123
                        .fill 12,96
                        .byte 123
                        .fill 12,96
                        .byte 179,0
fight_status_border_bottom:
                        .byte 173
                        .fill 12,96
                        .byte 177
                        .fill 12,96
                        .byte 177
                        .fill 12,96
                        .byte 189,0
fight_status_row_1:
                        .byte 98
                        .text "TR[K:       "
                        .byte 98
                        .text "RGSD:       "
                        .byte 98
                        .text "KORN:       "
                        .byte 98
                        .byte 0
fight_status_row_2:
                        .byte 98
                        .text "M[ND:       "
                        .byte 98
                        .text "SKYTS:      "
                        .byte 98
                        .text "REP:        "
                        .byte 98
                        .byte 0
ship_sank_msg:          .text "DIT SKIB GIK NED MED MAND OG MUS!"
                        .byte 0
end_sank_msg:           .text "BEKLAGER, DIT SKIB GIK NED !"
                        .byte 0
end_du_fik_msg:         .text "DU FIK"
                        .byte 0
end_point_og_havde_msg: .text "POINT OG HAVDE"
                        .byte 0
end_rigsdaler_msg:      .text "RIGSDALER!"
                        .byte 0
end_du_sluttede_msg:    .text "DU SLUTTEDE MED"
                        .byte 0
end_mand_og_msg:        .text "MAND OG"
                        .byte 0
end_rep_point_msg:      .text "REPERATIONS POINT"
                        .byte 0
resource_treasury_msg:  .text "DIT SKIB GIK NED: FOR MANGE PENGE!"
                        .byte 0
resource_crew_msg:      .text "DIT SKIB GIK NED: FOR MANGE FOLK!"
                        .byte 0
resource_cannons_msg:   .text "DIT SKIB GIK NED: FOR MANGE KANONER!"
                        .byte 0
resource_grain_msg:     .text "DIT SKIB GIK NED: FOR MEGET KORN!"
                        .byte 0
deadline_sorrow_msg:    .text "DET ER MED SORG, VI MEDDELER, AT"
                        .byte 0
deadline_komtesse_msg:  .text "KOMTESSE JULIE KNOKKELFRYD"
                        .byte 0
deadline_wedding_msg:   .text "IDAG FEJRER SIT BRYLLUP MED"
                        .byte 0
deadline_groom_msg:     .text "DEN VELB]RNE TULLEMAN GYLLEBORG!"
                        .byte 0
deadline_overboard_msg: .text "AF SORG HEROVER, HOPPER DU I HAVET!"
                        .byte 0
sharks_prefix_msg:      .text "DU SMIDER"
                        .byte 0
sharks_suffix_msg:      .text "MAND UD TIL HAJERNE!"
                        .byte 0
victory_msg:            .text "TILLYKKE!!!!"
                        .byte 0
victory_received_msg:   .text "DU HAR NETOP MODTAGET"
                        .byte 0
victory_flaskepost_msg: .text "DIN UDN[VNELSE MED FLASKEPOST."
                        .byte 0
victory_komtesse_msg:   .text "KOMTESSE JULIE VENTER DIG MED L[NGSEL!"
                        .byte 0
victory_next_climb_msg: .text "LAD OS SE, OM DU KAN KLARE DET"
                        .byte 0
victory_next_goal_prefix: .text "N[STE M]L "
                        .byte 0
victory_final_line_1:   .text "DU HAR KLARET DET HELE."
                        .byte 0
victory_final_line_2:   .text "DU F]R OGS] DET HALVE KONGERIGE"
                        .byte 0
boarding_battle_line_1: .text "KAMPEN RASER OMBORD P]"
                        .byte 0
map_label_trk:          .text "TR[K:"
                        .byte 0
map_label_rgsdl:        .text "RGSDL:"
                        .byte 0
map_label_skyts:        .text "SKYTS:"
                        .byte 0
map_label_maend:        .text "M[ND:"
                        .byte 0
map_label_point:        .text "POINT:"
                        .byte 0
map_label_korn:         .text "KORN:"
                        .byte 0
map_goal_prefix:        .text "M]L: "
                        .byte 0
map_goal_mid:           .text " P] "
                        .byte 0
map_goal_suffix:        .text " TR[K "
                        .byte 0
map_label_rep:          .text "REP:"
                        .byte 0
boarding_enemy_line_prefix:
                        .text "DEN ENGELSKE "
                        .byte 0
boarding_enemy_line_suffix:
                        .text " !"
                        .byte 0
boarding_pirate_line:   .text "S\R\VERSKIBET!"
                        .byte 0
boarding_enemy_losses_prefix:
                        .text "FJENDEN HAR "
                        .byte 0
boarding_enemy_losses_suffix:
                        .text " FALDNE,"
                        .byte 0
boarding_enemy_remaining_prefix:
                        .text "MEN HAN HAR "
                        .byte 0
boarding_enemy_remaining_suffix:
                        .text " IGEN."
                        .byte 0
boarding_player_losses_prefix:
                        .text "DU HAR MISTET "
                        .byte 0
boarding_player_losses_suffix:
                        .text " AF DINE TAPRE M[ND."
                        .byte 0
boarding_prompt_line_1: .text "VIL DU K[MPE VIDERE (K)"
                        .byte 0
boarding_prompt_line_2: .text "ELLER VENDE TILBAGE TIL DIT SKIB (T)?"
                        .byte 0
surrender_line_1:       .text "DE OVERGIVER SIG!!"
                        .byte 0
surrender_line_2:       .text "VIL DU S[NKE DET, (S)"
                        .byte 0
surrender_line_3:       .text "ELLER TAGE DET SOM PRISE (P) ?"
                        .byte 0
surrender_loot_rgsd_prefix:
                        .text "DER ER"
                        .byte 0
surrender_loot_rgsd_suffix:
                        .text "RIGSDALER OMBORD."
                        .byte 0
surrender_loot_men_suffix:
                        .text "OVERLEVENDE OMBORD."
                        .byte 0
surrender_loot_grain_suffix:
                        .text "S[KKE KORN OMBORD."
                        .byte 0
surrender_loot_take_line:
                        .text "DEM TAGER DU!"
                        .byte 0
surrender_prize_need_line_1:
                        .text "DETTE SKIB BEH\VER ET PRISEMANDSKAB"
                        .byte 0
surrender_prize_need_line_2_prefix:
                        .text "P]"
                        .byte 0
surrender_prize_need_line_2_suffix:
                        .text "MAND!"
                        .byte 0
surrender_prize_have_prefix:
                        .text "DU HAR"
                        .byte 0
surrender_prize_have_suffix:
                        .text "MAND."
                        .byte 0
surrender_prize_sailing_line:
                        .text "OK, DE ER P] VEJ TIL K\BENHAVN."
                        .byte 0

fog_line_2:             .text "OH\J!"
                        .byte 0
fog_line_3:             .text "DER ER EN UNDERLIG T]GEBANKE 2 S\MIL"
                        .byte 0
fog_line_4:             .text "TIL STYRBORD,"
                        .byte 0
fog_line_5:             .text "DET SER UD TIL, AT DER ER NOGET INDE I DEN."
                        .byte 0
fog_prompt_msg:         .text "VIL DU UNDERS\GE DET (J/N)?"
                        .byte 0
fog_enemy_line_1:       .text "DER DUKKER AF DISEN"
                        .byte 0
fog_enemy_line_2:       .text "ET V[MMELIGT SKIB!"
                        .byte 0
fog_found_island_msg:   .text "DU FINDER EN \DE \"
                        .byte 0
fog_treasure_line_2:    .text "MED EN SKATTEKISTE,"
                        .byte 0
fog_treasure_line_3:    .text "DER INDEHOLDER"
                        .byte 0
.label fog_treasure_line_4 = end_rigsdaler_msg
fog_jewel_line_2:       .text "MED"
                        .byte 0
fog_jewel_line_3:       .text "JUVELER!"
                        .byte 0
fog_cannon_line_2:      .text "HVOR DER LIGGER EN KANON!"
                        .byte 0
fog_crew_line_3:        .text "SKIBBRUDNE, SOM TAGER HYRE!"
                        .byte 0
fog_nothing_line_1:     .text "N[H!"
                        .byte 0
fog_nothing_line_2:     .text "DER VAR ALLIGEVEL IKKE NOGET."
                        .byte 0
fog_grain_line_3:       .text "S[KKE KORN!"
                        .byte 0
fog_plague_line_1_suffix:
                        .text "MED EN PESTSYG SKIBBRUDEN!"
                        .byte 0
fog_plague_line_2:      .text "HAN SMITTER DIN BES[TNING, OG EN TREDIEDEL D\R!"
                        .byte 0
fog_plague_line_3:      .text "RESTEN ER IKKE ALT FOR BEGEJSTREDE FOR DIT LEDERSKAB!"
                        .byte 0

naval_title_ptr_lo:
                        .byte <naval_title_1
                        .byte <naval_title_2
                        .byte <naval_title_3
                        .byte <naval_title_4
                        .byte <naval_title_5
naval_title_ptr_hi:
                        .byte >naval_title_1
                        .byte >naval_title_2
                        .byte >naval_title_3
                        .byte >naval_title_4
                        .byte >naval_title_5
noble_title_ptr_lo:
                        .byte <noble_title_1
                        .byte <noble_title_2
                        .byte <noble_title_3
                        .byte <noble_title_4
                        .byte <noble_title_5
noble_title_ptr_hi:
                        .byte >noble_title_1
                        .byte >noble_title_2
                        .byte >noble_title_3
                        .byte >noble_title_4
                        .byte >noble_title_5

naval_title_1:          .text "SKIBSF\RER"
                        .byte 0
naval_title_2:          .text "KAPTAJN"
                        .byte 0
naval_title_3:          .text "KOMMAND\RKAPTAJN"
                        .byte 0
naval_title_4:          .text "KOMMAND\R"
                        .byte 0
naval_title_5:          .text "ADMIRAL"
                        .byte 0

noble_title_1:          .text "BORGER "
                        .byte 0
noble_title_2:          .text "BARON "
                        .byte 0
noble_title_3:          .text "GREV "
                        .byte 0
noble_title_4:          .text "KAMMERHERRE "
                        .byte 0
noble_title_5:          .text "KRONPRINS "
                        .byte 0

enemy_name_ptr_table_lo:
                        .byte <enemy_name_1
                        .byte <enemy_name_2
                        .byte <enemy_name_3
                        .byte <enemy_name_4
                        .byte <enemy_name_5
                        .byte <enemy_name_6
                        .byte <enemy_name_7
                        .byte <enemy_name_8
enemy_name_ptr_table_hi:
                        .byte >enemy_name_1
                        .byte >enemy_name_2
                        .byte >enemy_name_3
                        .byte >enemy_name_4
                        .byte >enemy_name_5
                        .byte >enemy_name_6
                        .byte >enemy_name_7
                        .byte >enemy_name_8
enemy_cannons_table:
                        .byte 5,10,2,15,6,1,50,70
enemy_crew_table_lo:
                        .byte <80,<480,<40,<140,<50,<10,<140,<200
enemy_crew_table_hi:
                        .byte >80,>480,>40,>140,>50,>10,>140,>200
enemy_rgsd_table_lo:
                        .byte <150,<600,<150,<450,<200,<30,<900,<1500
enemy_rgsd_table_hi:
                        .byte >150,>600,>150,>450,>200,>30,>900,>1500
enemy_grain_table:
                        .byte 30,15,4,5,4,1,20,12

enemy_name_1:           .text "HANDELSMAND"
                        .byte 0
enemy_name_2:           .text "TROPPETRANSPORT"
                        .byte 0
enemy_name_3:           .text "KANONB]D"
                        .byte 0
enemy_name_4:           .text "GALEASE"
                        .byte 0
enemy_name_5:           .text "BRIG"
                        .byte 0
enemy_name_6:           .text "SKONNERT"
                        .byte 0
enemy_name_7:           .text "ORLOGSMAND"
                        .byte 0
enemy_name_8:           .text "S\R\VERSKIB"
                        .byte 0
harbour_copenhagen_msg: .text "K\BENHAVN"
                        .byte 0
harbour_helsingor_msg:  .text "HELSING\R"
                        .byte 0
harbour_hundested_msg:  .text "HUNDESTED"
                        .byte 0
harbour_grenaa_msg:     .text "GREN]"
                        .byte 0
harbour_ebeltoft_msg:   .text "EBELTOFT"
                        .byte 0
harbour_kalundborg_msg: .text "KALUNDBORG"
                        .byte 0
harbour_molle_msg:      .text "M\LLE"
                        .byte 0

decimal_divisor_lo:
                        .byte <10000,<1000,<100,<10,<1
decimal_divisor_hi:
                        .byte >10000,>1000,>100,>10,>1

// Bitmap base addresses per text row: $6000 + row * 320
bmp_row_base_lo:
        .byte <($6000+0*320),<($6000+1*320),<($6000+2*320),<($6000+3*320),<($6000+4*320)
        .byte <($6000+5*320),<($6000+6*320),<($6000+7*320),<($6000+8*320),<($6000+9*320)
        .byte <($6000+10*320),<($6000+11*320),<($6000+12*320),<($6000+13*320),<($6000+14*320)
        .byte <($6000+15*320),<($6000+16*320),<($6000+17*320),<($6000+18*320),<($6000+19*320)
        .byte <($6000+20*320),<($6000+21*320),<($6000+22*320),<($6000+23*320),<($6000+24*320)
bmp_row_base_hi:
        .byte >($6000+0*320),>($6000+1*320),>($6000+2*320),>($6000+3*320),>($6000+4*320)
        .byte >($6000+5*320),>($6000+6*320),>($6000+7*320),>($6000+8*320),>($6000+9*320)
        .byte >($6000+10*320),>($6000+11*320),>($6000+12*320),>($6000+13*320),>($6000+14*320)
        .byte >($6000+15*320),>($6000+16*320),>($6000+17*320),>($6000+18*320),>($6000+19*320)
        .byte >($6000+20*320),>($6000+21*320),>($6000+22*320),>($6000+23*320),>($6000+24*320)
// Color RAM base addresses per text row: $4400 + row * 40
clr_row_base_lo:
        .byte <($4400+0*40),<($4400+1*40),<($4400+2*40),<($4400+3*40),<($4400+4*40)
        .byte <($4400+5*40),<($4400+6*40),<($4400+7*40),<($4400+8*40),<($4400+9*40)
        .byte <($4400+10*40),<($4400+11*40),<($4400+12*40),<($4400+13*40),<($4400+14*40)
        .byte <($4400+15*40),<($4400+16*40),<($4400+17*40),<($4400+18*40),<($4400+19*40)
        .byte <($4400+20*40),<($4400+21*40),<($4400+22*40),<($4400+23*40),<($4400+24*40)
clr_row_base_hi:
        .byte >($4400+0*40),>($4400+1*40),>($4400+2*40),>($4400+3*40),>($4400+4*40)
        .byte >($4400+5*40),>($4400+6*40),>($4400+7*40),>($4400+8*40),>($4400+9*40)
        .byte >($4400+10*40),>($4400+11*40),>($4400+12*40),>($4400+13*40),>($4400+14*40)
        .byte >($4400+15*40),>($4400+16*40),>($4400+17*40),>($4400+18*40),>($4400+19*40)
        .byte >($4400+20*40),>($4400+21*40),>($4400+22*40),>($4400+23*40),>($4400+24*40)
screen_row_addr_lo:
                        .byte <INTRO_SCREEN_RAM,<(INTRO_SCREEN_RAM+$28),<(INTRO_SCREEN_RAM+$50),<(INTRO_SCREEN_RAM+$78),<(INTRO_SCREEN_RAM+$a0)
                        .byte <(INTRO_SCREEN_RAM+$c8),<(INTRO_SCREEN_RAM+$f0),<(INTRO_SCREEN_RAM+$118),<(INTRO_SCREEN_RAM+$140),<(INTRO_SCREEN_RAM+$168)
                        .byte <(INTRO_SCREEN_RAM+$190),<(INTRO_SCREEN_RAM+$1b8),<(INTRO_SCREEN_RAM+$1e0),<(INTRO_SCREEN_RAM+$208),<(INTRO_SCREEN_RAM+$230)
                        .byte <(INTRO_SCREEN_RAM+$258),<(INTRO_SCREEN_RAM+$280),<(INTRO_SCREEN_RAM+$2a8),<(INTRO_SCREEN_RAM+$2d0),<(INTRO_SCREEN_RAM+$2f8)
                        .byte <(INTRO_SCREEN_RAM+$320),<(INTRO_SCREEN_RAM+$348),<(INTRO_SCREEN_RAM+$370),<(INTRO_SCREEN_RAM+$398),<(INTRO_SCREEN_RAM+$3c0)
screen_row_addr_hi:
                        .byte >INTRO_SCREEN_RAM,>(INTRO_SCREEN_RAM+$28),>(INTRO_SCREEN_RAM+$50),>(INTRO_SCREEN_RAM+$78),>(INTRO_SCREEN_RAM+$a0)
                        .byte >(INTRO_SCREEN_RAM+$c8),>(INTRO_SCREEN_RAM+$f0),>(INTRO_SCREEN_RAM+$118),>(INTRO_SCREEN_RAM+$140),>(INTRO_SCREEN_RAM+$168)
                        .byte >(INTRO_SCREEN_RAM+$190),>(INTRO_SCREEN_RAM+$1b8),>(INTRO_SCREEN_RAM+$1e0),>(INTRO_SCREEN_RAM+$208),>(INTRO_SCREEN_RAM+$230)
                        .byte >(INTRO_SCREEN_RAM+$258),>(INTRO_SCREEN_RAM+$280),>(INTRO_SCREEN_RAM+$2a8),>(INTRO_SCREEN_RAM+$2d0),>(INTRO_SCREEN_RAM+$2f8)
                        .byte >(INTRO_SCREEN_RAM+$320),>(INTRO_SCREEN_RAM+$348),>(INTRO_SCREEN_RAM+$370),>(INTRO_SCREEN_RAM+$398),>(INTRO_SCREEN_RAM+$3c0)


