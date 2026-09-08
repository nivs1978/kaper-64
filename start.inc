// Title screen shown before the bitmap intro.

show_start_screen:
        jsr configure_intro_vic_bank
        jsr prepare_intro_charset

        lda #$93
        jsr $ffd2
        lda #$05
        jsr $ffd2

        // Centered on a 40-column screen: (40 - length) / 2.
        ldx #8
        ldy #0
        jsr start_plot
        pstrnl(start_title)

        ldx #11
        ldy #1
        jsr start_plot
        pstrnl(start_author)

        ldx #2
        ldy #3
        jsr start_plot
        pstrnl(start_port_author)

        ldx #10
        ldy #5
        jsr start_plot
        pstrnl(start_version)

        // Persisted record, centred (BASIC lines 42-45).
        ldx #17
        ldy #11
        jsr start_plot
        pstr(start_record_label)

        ldx #13
        ldy #13
        jsr start_plot
        pstr(start_record_points)
        pnum(highscore_value)

        ldx #8
        ldy #15
        jsr start_plot
        pstr(start_record_holder)
        pstr(highscore_name)

        ldx #0
        ldy #24
        jsr start_plot
        pstr(intro_press_key)   // no newline: printing past row 24 would scroll the screen

        // configure_intro_vic_bank blanks the screen; turn it back on once drawn.
        jsr show_vic_screen

start_wait_release:
        jsr $ff9f
        jsr $ffe4
        bne start_wait_release
start_wait_key:
        jsr $ff9f
        jsr $ffe4
        beq start_wait_key
        rts

start_plot:
        jmp set_cursor_xy       // X=column, Y=row

// -------------------------------------------------------------------------
// High score persistence - one 19-byte block (word score + zero-terminated
// name) in the disk file "REC.DAT" on device 8.
// -------------------------------------------------------------------------
.const HIGHSCORE_LOAD_NAME_LEN = highscore_filename_end - highscore_load_filename
.const HIGHSCORE_SCRATCH_NAME_LEN = highscore_filename_end - highscore_scratch_command
.const HIGHSCORE_DATA_SIZE = highscore_data_end - highscore_data
// KERNAL LOAD cannot be length-limited, so it targets the unused tape buffer
// and only HIGHSCORE_DATA_SIZE bytes are copied out: a stale or oversized
// REC.DAT would otherwise overrun the record block into the code behind it.
.const HIGHSCORE_LOAD_BUFFER = $0334

load_highscore:
        lda #0
        sta $9d                         // suppress SEARCHING/LOADING/error messages
        lda #HIGHSCORE_LOAD_NAME_LEN
        ldx #<highscore_load_filename
        ldy #>highscore_load_filename
        jsr $ffbd                       // SETNAM
        lda #$01
        ldx #$08
        ldy #$00                        // secondary address 0: load to the address in X/Y
        jsr $ffba                       // SETLFS
        lda #$00                        // 0 = load (not verify)
        ldx #<HIGHSCORE_LOAD_BUFFER
        ldy #>HIGHSCORE_LOAD_BUFFER
        jsr $ffd5                       // LOAD
        bcs reset_highscore_record
        ldx #0
load_highscore_copy:
        lda HIGHSCORE_LOAD_BUFFER,x
        sta highscore_data,x
        inx
        cpx #HIGHSCORE_DATA_SIZE
        bcc load_highscore_copy
        lda #0
        sta highscore_name + 16         // guarantee a terminator
        rts

// No record file yet (or a read error): fall back to the built-in defaults.
reset_highscore_record:
        lda #0
        sta highscore_value
        sta highscore_value+1
        ldx #0
reset_highscore_copy:
        lda highscore_default_name,x
        sta highscore_name,x
        beq reset_highscore_done
        inx
        cpx #16
        bcc reset_highscore_copy
        lda #0
        sta highscore_name,x
reset_highscore_done:
        rts

save_highscore:
        lda #0
        sta $9d                         // suppress SAVING/error messages
        // Scratch the old file first. Saving over it with an "@0:" replace
        // triggers the 1541's replace bug, which can corrupt the whole disk.
        lda #HIGHSCORE_SCRATCH_NAME_LEN
        ldx #<highscore_scratch_command
        ldy #>highscore_scratch_command
        jsr $ffbd                       // SETNAM
        lda #$0f
        ldx #$08
        ldy #$0f                        // secondary address 15: command channel
        jsr $ffba                       // SETLFS
        jsr $ffc0                       // OPEN sends the S0: command
        lda #$0f
        jsr $ffc3                       // CLOSE
        jsr $ffcc                       // CLRCHN

        lda #HIGHSCORE_LOAD_NAME_LEN
        ldx #<highscore_load_filename
        ldy #>highscore_load_filename
        jsr $ffbd                       // SETNAM
        lda #$01
        ldx #$08
        ldy #$00
        jsr $ffba                       // SETLFS
        lda #<highscore_data
        sta src_ptr
        lda #>highscore_data
        sta src_ptr+1
        ldx #<highscore_data_end
        ldy #>highscore_data_end
        lda #src_ptr                    // A = zero-page address of the start pointer
        jsr $ffd8                       // SAVE
        rts

// BASIC lines 1800-1809: beating the stored record rewrites it with the
// player's name and writes the file back to disk.
check_new_highscore:
        lda player_points+1
        cmp highscore_value+1
        bcc check_new_highscore_done
        bne check_new_highscore_record
        lda player_points
        cmp highscore_value
        bcc check_new_highscore_done
        beq check_new_highscore_done    // a tie does not take the record
check_new_highscore_record:
        lda player_points
        sta highscore_value
        lda player_points+1
        sta highscore_value+1
        ldx #0
check_new_highscore_copy:
        lda namebuf,x
        sta highscore_name,x
        beq check_new_highscore_stored
        inx
        cpx #16
        bcc check_new_highscore_copy
        lda #0
        sta highscore_name,x
check_new_highscore_stored:
        jsr save_highscore
        jsr print_newline
        pstrnl(new_record_msg)
check_new_highscore_done:
        rts

start_code_end:
// Pure text data, relocated into the free gap between the RLE unpacker and the
// harbour text so it does not eat into the sub-$4400 code budget.
* = $8300 "StartScreenText"

start_title:
        .text "KAPTAJN KAPER I KATTEGAT"
        .byte 0
start_author:
        .text "AF P.O.FREDERIKSEN"
        .byte 0
start_port_author:
        .text "COMMODORE 64 VERSION AF HANS MILLING"
        .byte 0
start_version:
        .text "VERSION 1 RELEASE 4"
        .byte 0
start_record_label:
        .text "REKORD"
        .byte 0
start_record_points:
        .text "POINT: "
        .byte 0
start_record_holder:
        .text "INDEHAVER: "
        .byte 0
new_record_msg:
        .text "NY REKORD!"
        .byte 0
highscore_default_name:
        .text "JOHAN N. HOLM"
        .byte 0
highscore_scratch_command:
        .text "S0:"                     // scratch, then save a fresh file
highscore_load_filename:
        .text "REC.DAT"
highscore_filename_end:

* = start_code_end
