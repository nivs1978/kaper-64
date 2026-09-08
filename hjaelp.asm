        // Kaper 64 - Kaptajn Kaper i Kattegat - standalone help
        // Original Author: P.O. Frederiksen
        // Commodore 64 port by Hans Milling
        // License: GNU General Public License v3.0; see LICENSE in the repository.

                BasicUpstart2(help_start)
                .encoding "petscii_upper"

        .const VIC_BANK0_SELECT = $03
        .const VIC_BANK1_SELECT = $02
        .const VIC_D018_MAIN = $15
        .const VIC_D018_INTRO = $12
        .const KERNAL_SCREEN_PAGE_ADDR = $0288
        .const KERNAL_SCREEN_PAGE_MAIN = $04
        .const KERNAL_SCREEN_PAGE_INTRO = $44
        .const INTRO_SCREEN_RAM = $4400
        .const INTRO_CHARSET_RAM = $4800
        .const SPRITE_PTR_TAB_BANK1_INTRO = $47f8
        .const SPRITE_DATA_BANK1 = $5000
        .const SPRITE_PTR_BANK1_BASE = 64
        .const CHAR_SLOT_AE_ADDR = INTRO_CHARSET_RAM + $00d8
        .const CHAR_SLOT_OE_ADDR = INTRO_CHARSET_RAM + $00e0
        .const CHAR_SLOT_AA_ADDR = INTRO_CHARSET_RAM + $00e8
        .label src_ptr = $fb
        .label dst_ptr = $fd

        .const HELP_PAGE_BREAK = $ff
        .const HELP_SHOOTING_SCREEN = $fe

                * = $0810

        help_start:
                jsr configure_help_screen
                jsr prepare_help_charset

        help_menu:
                jsr clear_help_screen
                lda #<help_menu_text
                sta src_ptr
                lda #>help_menu_text
                sta src_ptr+1
                jsr print_zero_terminated

        help_menu_key:
                jsr read_key_blocking
                cmp #'0'
                bcc leave_help
                cmp #':'
                bcs leave_help
                sec
                sbc #'0'
                tax
                lda help_topic_lo,x
                sta src_ptr
                lda help_topic_hi,x
                sta src_ptr+1
                jsr clear_help_screen
                jsr show_help_topic
                jmp help_menu

        leave_help:
                lda #0
                sta $d015
                jsr hide_vic_screen
                lda $dd00
                and #$fc
                ora #VIC_BANK0_SELECT
                sta $dd00
                lda #VIC_D018_MAIN
                sta $d018
                lda #KERNAL_SCREEN_PAGE_MAIN
                sta KERNAL_SCREEN_PAGE_ADDR
                lda $d011
                and #$df
                ora #$10
                sta $d011
                lda #$08
                sta $d016
                lda #14
                sta $d020
                lda #6
                sta $d021
                lda #$93
                jsr $ffd2
                jmp $a474

        show_help_topic:
                ldy #0
        show_help_next:
                lda (src_ptr),y
                beq show_help_done
                jsr advance_src_ptr
                cmp #HELP_PAGE_BREAK
                beq show_help_new_page
                cmp #HELP_SHOOTING_SCREEN
                beq show_help_shooting
                jsr $ffd2
                jmp show_help_next

        show_help_new_page:
                jsr wait_for_continue
                jsr clear_help_screen
                jmp show_help_next

        show_help_shooting:
                jsr wait_for_continue
                lda src_ptr
                pha
                lda src_ptr+1
                pha
                jsr show_shooting_help
                jsr wait_for_key_only
                jsr clear_help_screen
                pla
                sta src_ptr+1
                pla
                sta src_ptr
                jmp show_help_next

        show_help_done:
                jsr wait_for_continue
                rts

        advance_src_ptr:
                inc src_ptr
                bne advance_src_done
                inc src_ptr+1
        advance_src_done:
                rts

        wait_for_continue:
                lda #13
                jsr $ffd2
                lda #<continue_text
                sta wait_saved_lo
                lda #>continue_text
                sta wait_saved_hi
                lda src_ptr
                pha
                lda src_ptr+1
                pha
                lda wait_saved_lo
                sta src_ptr
                lda wait_saved_hi
                sta src_ptr+1
                jsr print_zero_terminated
                pla
                sta src_ptr+1
                pla
                sta src_ptr
        wait_for_key_only:
                lda #0
                sta $c6
                jmp read_key_blocking

        read_key_blocking:
                jsr $ffe4
                beq read_key_blocking
                rts

        clear_help_screen:
                lda #0
                sta $d015
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
                inc src_ptr+1
                jmp print_zero_terminated_loop
        print_zero_terminated_done:
                rts

        configure_help_screen:
                jsr hide_vic_screen
                lda $dd00
                and #$fc
                ora #VIC_BANK1_SELECT
                sta $dd00
                lda #VIC_D018_INTRO
                sta $d018
                lda #KERNAL_SCREEN_PAGE_INTRO
                sta KERNAL_SCREEN_PAGE_ADDR
                lda $d011
                and #$df
                sta $d011
                lda $d016
                and #$ef
                sta $d016
                lda #14
                sta $d020
                lda #6
                sta $d021
                jsr show_vic_screen
                rts

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

        prepare_help_charset:
                sei
                lda $01
                pha
                and #$fb
                sta $01
                lda #$00
                sta src_ptr
                sta dst_ptr
                lda #$d0
                sta src_ptr+1
                lda #>INTRO_CHARSET_RAM
                sta dst_ptr+1
                ldx #8
                ldy #0
        help_charset_copy:
                lda (src_ptr),y
                sta (dst_ptr),y
                iny
                bne help_charset_copy
                inc src_ptr+1
                inc dst_ptr+1
                dex
                bne help_charset_copy
                pla
                sta $01
                cli

                ldy #7
        help_charset_patch:
                lda char_ae,y
                sta CHAR_SLOT_AE_ADDR,y
                lda char_oe,y
                sta CHAR_SLOT_OE_ADDR,y
                lda char_aa,y
                sta CHAR_SLOT_AA_ADDR,y
                dey
                bpl help_charset_patch
                rts

        show_shooting_help:
                jsr hide_vic_screen
                lda #<shooting_screen_data
                sta src_ptr
                lda #>shooting_screen_data
                sta src_ptr+1
                lda #<INTRO_SCREEN_RAM
                sta dst_ptr
                lda #>INTRO_SCREEN_RAM
                sta dst_ptr+1
                jsr copy_1000_bytes
                lda #<shooting_color_data
                sta src_ptr
                lda #>shooting_color_data
                sta src_ptr+1
                lda #<$d800
                sta dst_ptr
                lda #>$d800
                sta dst_ptr+1
                jsr copy_1000_bytes

                ldy #63
        help_crosshair_copy:
                lda help_crosshair_sprite,y
                sta SPRITE_DATA_BANK1,y
                dey
                bpl help_crosshair_copy
                lda #SPRITE_PTR_BANK1_BASE
                sta SPRITE_PTR_TAB_BANK1_INTRO
                lda #244
                sta $d000
                lda #90
                sta $d001
                lda $d010
                and #$fe
                sta $d010
                lda #10
                sta $d027
                lda #1
                sta $d015
                ldx #6
                ldy #17
                lda #<help_stat_distance
                jsr print_help_field
                ldx #14
                ldy #19
                lda #<help_stat_own_cannons
                jsr print_help_field
                ldx #14
                ldy #21
                lda #<help_stat_own_crew
                jsr print_help_field
                ldx #14
                ldy #23
                lda #<help_stat_own_repairs
                jsr print_help_field
                ldx #27
                ldy #19
                lda #<help_stat_enemy_cannons
                jsr print_help_field
                ldx #27
                ldy #21
                lda #<help_stat_enemy_crew
                jsr print_help_field
                ldx #27
                ldy #23
                lda #<help_stat_enemy_repairs
                jsr print_help_field
                ldx #17
                ldy #15
                lda #<help_shooting_prompt
                jsr print_help_field
                jsr show_vic_screen
                rts

        // All sample strings share a page; A supplies the low address byte.
        print_help_field:
                sta src_ptr
                lda #>help_stat_distance
                sta src_ptr+1
                txa
                pha
                tya
                tax
                pla
                tay
                clc
                jsr $fff0
                jmp print_zero_terminated

        copy_1000_bytes:
                ldy #0
                ldx #3
        help_copy_page:
                lda (src_ptr),y
                sta (dst_ptr),y
                iny
                bne help_copy_page
                inc src_ptr+1
                inc dst_ptr+1
                dex
                bne help_copy_page
                ldy #0
        help_copy_tail:
                lda (src_ptr),y
                sta (dst_ptr),y
                iny
                cpy #232
                bne help_copy_tail
                rts

        wait_saved_lo: .byte 0
        wait_saved_hi: .byte 0

        char_ae: .byte 63,108,108,127,108,108,111,0
        char_oe: .byte 60,102,110,126,118,102,60,0
        char_aa: .byte 24,36,24,60,102,126,102,0

        help_crosshair_sprite:
                .byte $00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00
                .byte $00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00
                .byte $ff,$ff,$fe
                .byte $00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00
                .byte $00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00,$00,$10,$00
                .byte $0a

        help_stat_distance:      .text "520"
                                 .byte 0
        help_stat_own_cannons:   .text "17"
                                 .byte 0
        help_stat_own_crew:      .text "97"
                                 .byte 0
        help_stat_own_repairs:   .text "142"
                                 .byte 0
        help_stat_enemy_cannons: .text "12"
                                 .byte 0
        help_stat_enemy_crew:    .text "36"
                                 .byte 0
        help_stat_enemy_repairs: .text "72"
                                 .byte 0
        help_shooting_prompt:    .text "TRYK EN TAST"
                                 .byte 0

        help_topic_lo:
                .byte <help_topic_info,<help_topic_general,<help_topic_navigation
                .byte <help_topic_harbours,<help_topic_cannons,<help_topic_boarding
                .byte <help_topic_crew,<help_topic_prizes,<help_topic_trade,<help_topic_fog
        help_topic_hi:
                .byte >help_topic_info,>help_topic_general,>help_topic_navigation
                .byte >help_topic_harbours,>help_topic_cannons,>help_topic_boarding
                .byte >help_topic_crew,>help_topic_prizes,>help_topic_trade,>help_topic_fog

        help_menu_text:
                .text "HVILKET EMNE \NSKER DU AT H\RE OM:"
                .byte 13,13
                .text " 1: GENERELLE FORUDS[TNINGER"
                .byte 13
                .text " 2: NAVIGATION"
                .byte 13
                .text " 3: HAVNE"
                .byte 13
                .text " 4: KAMP MED KANONER"
                .byte 13
                .text " 5: KAMP VED AT BORDE"
                .byte 13
                .text " 6: BES[TNINGENS R\GT OG PLEJE"
                .byte 13
                .text " 7: PRISER, PRISEPENGE OG MANDSKAB"
                .byte 13
                .text " 8: HANDEL"
                .byte 13
                .text " 9: T]GE, SYGDOM OG SKATTEJAGT"
                .byte 13
                .text " 0: PROGRAMINFORMATION"
                .byte 13,13
                .text "V[LG 0-9. EN ANDEN TAST AFSLUTTER."
                .byte 0

        continue_text:
                .text "TRYK P] EN TAST FOR AT FORTS[TTE!"
                .byte 0

        help_topic_general:
                .text "NU M] DU T[NKE DIG TILBAGE TIL TIDEN"
                .byte 13
                .text "LIGE EFTER SLAGET P] REDEN."
                .byte 13,13
                .text "ENGELSKMANDEN HAR R\VET DE FLESTE AF"
                .byte 13
                .text "VORE SKIBE; KUN EN H]NDFULD SM]SKIBE -"
                .byte 13
                .text "DERIBLANDT DIT - ER TILBAGE."
                .byte 13,13
                .text "DE ENGELSKE SKIBE SEJLER, SOM DE"
                .byte 13
                .text "LYSTER P] VORE SUNDE OG B[LTER."
                .byte 13,13
                .text "KONGEN HAR GIVET DIG ET KAPERBREV,"
                .byte 13
                .text "SOM GIVER DIG RET TIL AT KAPRE ALLE DE"
                .byte 13
                .text "ENGELSKE SKIBE, DU ST\DER P]."
                .byte 13,13
                .text "DU F]R UDBETALT EN ST\RRE SUM I"
                .byte 13
                .text "PRISEPENGE FOR HVERT SKIB, DU F]R"
                .byte 13
                .text "AFLEVERET I K\BENHAVNS HAVN."
                .byte 13,13
                .text "DER, HVOR DU HAR NEDK[MPET EN FJENDE,"
                .byte 13
                .text "T\R ENGL[NDERNE IKKE KOMME MERE!"
                .byte 13,13
                .text "DET G[LDER OGS] S\R\VERE!"
                .byte HELP_PAGE_BREAK
                .text "DU BLIVER BEL\NNET MED POINT FOR"
                .byte 13
                .text "AT SKADE ENGL[NDERNE, OG HVIS DU F]R"
                .byte 13
                .text "POINT NOK, VIL KONGEN ADLE DIG."
                .byte 13,13
                .text "N]R DU ER BLEVET ADELIG, KAN DIN DR\M"
                .byte 13
                .text "G] I OPFYLDELSE:"
                .byte 13
                .text "DU KAN BLIVE GIFT MED DIN HEMMELIGE"
                .byte 13
                .text "FORLOVEDE: KOMTESSE JULIE KNOKKELFRYD"
                .byte 13
                .text "TIL KNOKKELHOLM."
                .byte 13,13
                .text "MEN DU M] SKYNDE DIG!!"
                .byte 13,13
                .text "JUNKER TULLEMAND TIL GYLLEBORG ER"
                .byte 13
                .text "LUN P] KOMTESSEN, OG DERSOM DU IKKE"
                .byte 13
                .text "N]R AT BLIVE ADLET HURTIGT NOK, VIL"
                .byte 13
                .text "BARON KNOKKELFRYD SK[NKE SIN DATTERS"
                .byte 13
                .text "H]ND TIL DEN VELBESL[EDE JUNKER!"
                .byte 13,13
                .text "DERFOR: FRISK MOD OG GOD VIND!"
                .byte 13
                .text "OG HOLD DIT KRUDT T\RT!"
                .byte 0

        help_topic_navigation:
                .text "DU KAN SE DIG SELV P] KORTET OVER"
                .byte 13
                .text "HAVET OMKRING DANMARK. DU ER VIST SOM"
                .byte 13
                .text "ET LILLE SKIB, DER GYNGER P] VANDET."
                .byte 13,13
                .text "DU KAN FLYTTE DIG I 8 RETNINGER VED AT"
                .byte 13
                .text "BRUGE 1-4 OG 6-9 (PILTASTERNE)."
                .byte 13,13
                .text "DU KAN IKKE SEJLE UDENFOR KORTET, OG"
                .byte 13
                .text "DU KAN IKKE SEJLE OP P] LAND,"
                .byte 13
                .text "MEN M]SKE SER DET UD, SOM OM DU"
                .byte 13
                .text "AF OG TIL KUNNE SEJLE OVER LANDTANGER."
                .byte 13,13
                .text "FORS\GER DU AT SEJLE OP P] LAND,"
                .byte 13
                .text "SKER DER SKADE P] BUNDEN AF DIT SKIB."
                .byte 13,13
                .text "FORS\GER DU AT SEJLE UD AF KORTET,"
                .byte 13
                .text "BLIVER DU BARE FORHINDRET I DET."
                .byte 0

        help_topic_harbours:
                .text "DER ER VIST 7 HAVNE P] KORTET."
                .byte 13,13
                .text "N]R DU SEJLER IND I ET KVADRAT, HVOR"
                .byte 13
                .text "DER ER EN HAVN, SKAL DU ANL\BE HAVNEN."
                .byte 13,13
                .text "DU SKAL STYRE FORBI ALLE DE SKIBE, DER"
                .byte 13
                .text "LIGGER P] REDEN FORAN HAVNEINDL\BET."
                .byte 13
                .text "DU BRUGER 4 OG 6 TIL AT STYRE MED."
                .byte 13,13
                .text "HVIS DU ST\DER SAMMEN MED ET AF DE"
                .byte 13
                .text "ANDRE SKIBE, SKER DER SKADE P] DIT"
                .byte 13
                .text "SKIB. RAMMER DU IKKE HAVNEINDL\BET,"
                .byte 13
                .text "S] SYNKER DIT SKIB, OG SPILLET ER SLUT!"
                .byte 13,13
                .text "PAS P] DE PLUDSELIGE VINDST\D, DER KAN"
                .byte 13
                .text "SL] DIT SKIB UD AF KURS."
                .byte 0

        help_topic_cannons:
                .text "N]R DU M\DER EN FJENDE KAN DU V[LGE"
                .byte 13
                .text "AT K[MPE ELLER AT FLYGTE."
                .byte 13
                .text "JO OFTERE DU FLYGTER, DES D]RLIGERE"
                .byte 13
                .text "BLIVER MANDSKABETS MORAL OG KAMPEVNE."
                .byte 13,13
                .text "HVIS DU V[LGER AT K[MPE MED KANONER,"
                .byte 13
                .text "VIL DU F] VIST ET BILLEDE AF FJENDENS"
                .byte 13
                .text "SKIB, SOM DU SKAL SIGTE EFTER."
                .byte 13,13
                .text "SIGTET, DER ER VIST SOM ET KORS, KAN"
                .byte 13
                .text "FLYTTES MED PILTASTERNE."
                .byte 13,13
                .text "DU F]R LIGE BILLEDET AT SE P]."
                .byte HELP_SHOOTING_SCREEN
                .text "FOROVEN TIL VENSTRE KAN DU SE, HVORDAN"
                .byte 13
                .text "DU RAMMER I FORHOLD TIL HAM, OG HVORDAN"
                .byte 13
                .text "HAN RAMMER I FORHOLD TIL DIG."
                .byte 13,13
                .text "DET G[LDER DOG KUN I BEGYNDELSEN."
                .byte 13
                .text "SENERE M] DU N\JES MED DET PLASK,"
                .byte 13
                .text "DU KAN SE \VERST TIL H\JRE."
                .byte 13
                .text "DET ER DER KUN ET \JEBLIK, OG DU KAN"
                .byte 13
                .text "IKKE SE DET, HVIS DET ER BAG HANS SKIB."
                .byte 13,13
                .text "SE LIGE P] BILLEDET IGEN."
                .byte HELP_SHOOTING_SCREEN
                .text "FORNEDEN TIL VENSTRE SER DU AFSTAND,"
                .byte 13
                .text "VINDRETNING OG VINDHASTIGHED, SOM ALLE"
                .byte 13
                .text "HAR BETYDNING FOR, HVORDAN DU SIGTER."
                .byte 13,13
                .text "HVIS DU IKKE SYNES OM AFSTANDEN, KAN DU"
                .byte 13
                .text "AFBRYDE KAMPEN (0) OG ANGRIBE IGEN."
                .byte 13,13
                .text "DU KAN OGS] AFBRYDE KAMPEN, FORDI DU"
                .byte 13
                .text "ER VED AT TABE ELLER VIL BORDE SKIBET."
                .byte 13,13
                .text "SE LIGE P] BILLEDET IGEN."
                .byte HELP_SHOOTING_SCREEN
                .text "SKEMAET FORNEDEN P] BILLEDET VISER DIG"
                .byte 13
                .text "DIN STYRKE I FORHOLD TIL HANS."
                .byte 13,13
                .text "JO FLERE KANONER, DU HAR, DES MERE"
                .byte 13
                .text "SKADE SKER DER P] HAM, N]R DU RAMMER."
                .byte 13
                .text "OG OMVENDT, SELVF\LGELIG!"
                .byte 13,13
                .text "DU KAN F\LGE UDVIKLINGEN I SKEMAET."
                .byte 13
                .text "UNDER 20 REPARATIONSPOINT SYNKER SKIBET."
                .byte 13
                .text "MED UNDER 10 MAND MISTER DU KONTROLLEN,"
                .byte 13
                .text "OG S] SYNKER DU OGS]!"
                .byte 0

        help_topic_boarding:
                .text "HVIS DU V[LGER AT BORDE FJENDEN,"
                .byte 13
                .text "VIL ET ANTAL AF HANS OG DINE M[ND D\!"
                .byte 13
                .text "ANTALLET AFH[NGER BL.A. AF DIT"
                .byte 13
                .text "MANDSKABS MORAL."
                .byte 13,13
                .text "HVIS DER IKKE ER RET MANGE FJENDER"
                .byte 13
                .text "IGEN, VIL DE OVERGIVE SIG. ELLERS M] DU"
                .byte 13
                .text "FORTS[TTE KAMPEN ELLER TR[KKE DIG"
                .byte 13
                .text "TILBAGE TIL DIT SKIB FOR AT FLYGTE"
                .byte 13
                .text "ELLER BRUGE KANONERNE."
                .byte 13,13
                .text "EN FORDEL VED AT BORDE ER, AT SKIBET"
                .byte 13
                .text "ALDRIG SYNKER. DU F]R DERFOR ALTID"
                .byte 13
                .text "LAGT DIN KLAMME H]ND P] DERES PENGE."
                .byte 13,13
                .text "N]R DE HAR OVERGIVET SIG, KAN DU S[NKE"
                .byte 13
                .text "SKIBET, S] DE OVERLEVENDE SLUTTER SIG"
                .byte 13
                .text "TIL DIG, ELLER TAGE SKIBET SOM PRISE."
                .byte 13
                .text "DET ST]R DER MERE OM UNDER PUNKT 7."
                .byte 0

        help_topic_crew:
                .text "DINE FOLK KAN BLIVE DR[BT AF FJENDEN."
                .byte 13
                .text "DET KAN SKE B]DE VED KANONDUELLER,"
                .byte 13
                .text "OG N]R DU BORDER FJENDEN."
                .byte 13,13
                .text "BES[TNINGEN SPISER KORN FOR HVERT TR[K."
                .byte 13
                .text "SLIPPER KORNET OP, BEGYNDER MANDSKABET"
                .byte 13
                .text "AT D\ AF SULT. S] ER DET P] H\JE TID"
                .byte 13
                .text "AT F] K\BT MERE KORN (SE PUNKT 8)."
                .byte 13,13
                .text "BES[TNINGEN KAN OGS] D\ AF SYGDOM."
                .byte 13
                .text "DET KAN DU L[SE OM I PUNKT 9."
                .byte 13,13
                .text "HUSK AT BES[TNINGENS MORAL OG KAMPEVNE"
                .byte 13
                .text "FALDER HVER GANG, DU FLYGTER ELLER"
                .byte 13
                .text "OPGIVER EN KAMP, MEN DEN STIGER,"
                .byte 13
                .text "HVER GANG DU VINDER."
                .byte 0

        help_topic_prizes:
                .text "N]R DU TAGER EN PRISE, SKAL DU"
                .byte 13
                .text "AFLEVERE DEN I K\BENHAVN."
                .byte 13,13
                .text "DU S[TTER ET PRISEMANDSKAB OMBORD P]"
                .byte 13
                .text "SKIBET. DET SKAL BRINGE SKIB OG FANGER"
                .byte 13
                .text "VELBEHOLDNE TIL HOVEDSTADEN."
                .byte 13,13
                .text "HVOR MANGE MAND, DER SKAL TIL, AFH[NGER"
                .byte 13
                .text "AF SKIBETS ST\RRELSE OG AF, HVOR MANGE"
                .byte 13
                .text "FANGER DER ER AT PASSE P]."
                .byte 13
                .text "DU VIL F] AT VIDE, HVOR MANGE DET ER."
                .byte 13,13
                .text "DET ER IKKE ALTID, AT PRISEN N]R FREM;"
                .byte 13
                .text "AF OG TIL FORSVINDER DEN UNDERVEJS,"
                .byte 13
                .text "OG HVERKEN SKIB ELLER MANDSKAB BLIVER"
                .byte 13
                .text "SET IGEN. MEN FOR DET MESTE G]R DET GODT."
                .byte 13
                .text "N[STE GANG DU KOMMER TIL K\BENHAVN,"
                .byte 13
                .text "VENTER DINE FOLK MED ET P[NT BEL\B"
                .byte 13
                .text "I PRISEPENGE."
                .byte 0

        help_topic_trade:
                .text "I HAVNEBYERNE KAN DU K\BE OG S[LGE."
                .byte 13,13
                .text "DU KAN F] REPARERET SKIBET OG HYRE"
                .byte 13
                .text "MERE MANDSKAB. DU KAN K\BE KORN OG"
                .byte 13
                .text "KANONER OG S[LGE KORN, KANONER OG"
                .byte 13
                .text "JUVELER."
                .byte 13,13
                .text "PRISERNE ER I RIGSDALER. DE SVINGER"
                .byte 13
                .text "FRA STED TIL STED OG FRA DAG TIL DAG,"
                .byte 13
                .text "MEN DER KAN IKKE FORHANDLES OM DEM."
                .byte 13,13
                .text "DU KAN S[LGE KORN FOR AT K\BE KANONER,"
                .byte 13
                .text "ELLER OMVENDT, MEN IKKE DIT MANDSKAB."
                .byte 13
                .text "JUVELER ER SJ[LDNE OG KAN IKKE K\BES."
                .byte 13
                .text "DE KAN KUN FINDES P] HEMMELIGE STEDER."
                .byte HELP_PAGE_BREAK
                .text "DU SKAL IKKE K\BE FOR MEGET, FOR DIT"
                .byte 13
                .text "SKIB KAN IKKE B[RE MERE END"
                .byte 13
                .text "150 KANONER OG 500 MAND."
                .byte 13,13
                .text "PENGE VEJER OGS] EN DEL. PR\V IKKE AT"
                .byte 13
                .text "SEJLE MED MERE END 30000 RIGSDALER."
                .byte 13,13
                .text "DET ER HELLER IKKE KLOGT AT HAVE"
                .byte 13
                .text "MERE END 700 S[KKE KORN OMBORD."
                .byte 0

        help_topic_fog:
                .text "AF OG TIL VIL DU ST\DE P] EN MYSTISK"
                .byte 13
                .text "T]GEBANKE, SOM DU KAN UNDERS\GE ELLER"
                .byte 13
                .text "V[LGE AT UNDG]."
                .byte 13,13
                .text "DET, DER EVENTUELT ER I DEN, KAN V[RE"
                .byte 13
                .text "GODT, MEN DET KAN OGS] V[RE SKIDT."
                .byte 13,13
                .text "B]DE JUVELER OG BYLDEPEST ER"
                .byte 13
                .text "INDEN FOR MULIGHEDERNES R[KKEVIDDE."
                .byte 0

        help_topic_info:
                .text "KAPTAJN KAPER I KATTEGAT"
                .byte 13,13
                .text "ORIGINALT PROGRAM:"
                .byte 13
                .text "P.O. FREDERIKSEN"
                .byte 13,13
                .text "COMMODORE 64-PORT:"
                .byte 13
                .text "HANS MILLING"
                .byte 13,13
                .text "DETTE PROGRAM M] GERNE KOPIERES."
                .byte 13
                .text "HVIS DU KENDER NOGEN, DER KAN HAVE"
                .byte 13
                .text "GL[DE AF DET, S] LAD DEM F] EN KOPI."
                .byte 13,13
                .text "LICENS: GNU GPL VERSION 3."
                .byte 13
                .text "SE LICENSE-FILEN I KILDEKODEN."
                .byte 0

        .errorif (* > INTRO_SCREEN_RAM), "Help code and text overrun screen RAM at $4400"
        help_code_end:

                * = $c000 "ShootingScreenData"
                .import source "shooting_screen_data.inc"
