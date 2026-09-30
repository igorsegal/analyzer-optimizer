# STPatterns - LIMB

Р РµРµСЃС‚СЂ РёР·РјРµРЅРµРЅРёР№ РїСЂРѕРµРєС‚Р° STPatterns. Append-only.

## РџСЂР°РІРёР»Р°

R1. Р›СЋР±РѕРµ РёР·РјРµРЅРµРЅРёРµ РєРѕРґР° С„РёРєСЃРёСЂСѓРµС‚СЃСЏ Р·Р°РїРёСЃСЊСЋ. Р‘РµР· Р·Р°РїРёСЃРё вЂ” РЅРµ РєРѕРјРјРёС‚РёС‚СЃСЏ.
R2. РќРѕРјРµСЂ LIMB-NNNN СЂР°СЃС‚С‘С‚ РїРѕСЃР»РµРґРѕРІР°С‚РµР»СЊРЅРѕ. РќРµ РїСЂРѕРїСѓСЃРєР°РµС‚СЃСЏ.
R3. РџСЂРѕС€Р»С‹Рµ Р·Р°РїРёСЃРё РЅРµ СЂРµРґР°РєС‚РёСЂСѓСЋС‚СЃСЏ.
R4. РљР°Р¶РґС‹Р№ РјРѕРґСѓР»СЊ вЂ” РѕС‚РґРµР»СЊРЅР°СЏ Р·Р°РїРёСЃСЊ РїСЂРё СЃРѕР·РґР°РЅРёРё.
R5. Р—Р°РїРёСЃРё РІ РѕР±СЂР°С‚РЅРѕРј РїРѕСЂСЏРґРєРµ: СЃРІРµР¶РёРµ СЃРІРµСЂС…Сѓ.

## Р¤РѕСЂРјР°С‚

## LIMB-0019 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp,
              stpatterns/tools/st_emulator_main.cpp
- Type:       LOGIC
- Reason:     (1) main.cpp reads point from file header
                  (XFBarReader::header().point) instead of hardcoded 0.00001.
              (2) threshold computed as 12% of ADR(5) in local points.
                  For EURUSD ~60 pips ADR -> ~7 pips threshold (author).
                  For other instruments scales automatically.
              (3) adr5_pts now computed from real daily range in local points.
              (4) MAX_H filter moved to pct of ADR (12-50%) to allow threshold < max.
- Impact:     Multi-symbol scan gives meaningful numbers.
- Rollback:   LIMB-0018.
- Status:     ACTIVE## LIMB-0018 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp,
              stpatterns/tools/st_emulator_main.cpp
- Type:       LOGIC
- Reason:     (1) find_previous_direction rewritten: check both pairs
                  (HH and LL) independently, prefer the more recent one.
                  Was returning 0 in 53% of cases, now should be defined
                  most of the time.
              (2) main.cpp scans a directory recursively (all *_H1.bin)
                  and prints per-symbol summary plus aggregated bucket
                  breakdown by corridor size (pct of ADR).
- Impact:     prev-direction rule activates; multi-symbol overview.
- Rollback:   LIMB-0017.
- Status:     ACTIVE## LIMB-0017 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Two fixes to match author literally:
              (1) TP and BE triggers measured from start_line (broken
                  fractal), not from entry_price. Book page 45:
                  TP = 400% of height from broken fractal;
                  BE trigger at 210% of height from broken fractal.
              (2) Session filter (page 80): trade only 06:00-19:00 GMT,
                  skip Friday after 20:00 GMT, skip Monday before 02:00.
- Impact:     Different exit levels, filtered session.
- Rollback:   LIMB-0016.
- Status:     ACTIVE## LIMB-0014 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Two bugs fixed:
              (1) find_not_fully_formed_stop had swapped conditions.
                  BUY must check LOW (potential DOWN fractal),
                  SELL must check HIGH (potential UP fractal).
              (2) find_previous_direction required both UP and DOWN
                  pairs to be unambiguous. Now uses the LAST fractal
                  before the corridor to pick which pair to check.
- Impact:     Previous-direction and not-fully-formed stop may
              finally activate.
- Rollback:   LIMB-0012 logic.
- Status:     ACTIVE
## LIMB-0014 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Two bugs fixed:
              (1) find_not_fully_formed_stop had swapped conditions.
                  BUY must check LOW (potential DOWN fractal),
                  SELL must check HIGH (potential UP fractal).
              (2) find_previous_direction required both UP and DOWN
                  pairs to be unambiguous. Now uses the LAST fractal
                  before the corridor to pick which pair to check.
- Impact:     Previous-direction and not-fully-formed stop may
              finally activate.
- Rollback:   LIMB-0012 logic.
- Status:     ACTIVE## LIMB-0012 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Implement author rules (book pages 38-43):
              (1) previous_direction: compare last two up / last two down
                  fractals before the corridor.
              (2) reversal_zone: last closed bar before break.
              (3) stop selection: if trade direction matches
                  previous_direction -> try not-fully-formed opposite
                  fractal at bar[break-1]; else use last fully-formed
                  opposite fractal.
- Impact:     Stop may be closer in trend-following setups.
- Rollback:   Use only fully-formed stop (LIMB-0008 logic).
- Status:     ACTIVE## LIMB-NNNN - YYYY-MM-DD - STP
- Р¤Р°Р№Р»С‹:      СЃРїРёСЃРѕРє
- РўРёРї:        CONTRACT | MODULE | LOGIC | CONFIG
- РџСЂРёС‡РёРЅР°:    С‚РµРєСЃС‚
- Р’Р»РёСЏРЅРёРµ:    С‡С‚Рѕ Р·Р°С‚СЂРѕРЅСѓС‚Рѕ
- РћС‚РєР°С‚:      РєР°Рє РѕС‚РєР°С‚РёС‚СЊ
- РЎС‚Р°С‚СѓСЃ:     ACTIVE | REVERTED | SUPERSEDED

---

## LIMB-0010 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Bug in simulate_exit: BE trigger was checked before TP.
              If a bar's range covered both +210% (BE trigger) and
              +400% (TP), we exited at BE instead of TP. Result:
              wins dropped from 391 to 319, PF from 0.955 to 0.812.
              Fix: check TP first, then SL/BE.
- Impact:     TP hits preserved. WR should stay ~25-30%.
- Rollback:   Move BE check back before TP.
- Status:     ACTIVE
## LIMB-0010 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Bug in simulate_exit: BE trigger was checked before TP.
              If a bar's range covered both +210% (BE trigger) and
              +400% (TP), we exited at BE instead of TP. Result:
              wins dropped from 391 to 319, PF from 0.955 to 0.812.
              Fix: check TP first, then SL/BE.
- Impact:     TP hits preserved. WR should stay ~25-30%.
- Rollback:   Move BE check back before TP.
- Status:     ACTIVE
## LIMB-0009 - 2026-09-30 - STP
- Files:      stpatterns/src/corridor.cpp
- Type:       LOGIC
- Reason:     Add BE-at-210 rule (author, page 45):
              when price moves 2.1 * height into profit,
              move SL to entry price (breakeven).
              Without this rule trades that move +210% and reverse
              become full losses. This is why WR was only 25%.
- Impact:     Expected WR up to 35-50%, PF > 1.0
- Rollback:   Remove BE logic from simulate_exit
- Status:     ACTIVE## LIMB-0008 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/src/corridor.cpp
- РўРёРї:        LOGIC
- РџСЂРёС‡РёРЅР°:    Р‘Р°Рі stop_line: РёСЃРїРѕР»СЊР·РѕРІР°Р»СЃСЏ prev (РїСЂРµРґС‹РґСѓС‰РёР№ РІ СЃРїРёСЃРєРµ),
              Р° РЅСѓР¶РµРЅ Р±Р»РёР¶Р°Р№С€РёР№ РїСЂРѕС‚РёРІРѕРїРѕР»РѕР¶РЅС‹Р№ С„СЂР°РєС‚Р°Р» РќРЈР–РќРћР™ СЃС‚РѕСЂРѕРЅС‹.
              Р”Р»СЏ BUY (curr UP): stop = Р±Р»РёР¶Р°Р№С€РёР№ РїСЂРµРґС‹РґСѓС‰РёР№ DOWN-С„СЂР°РєС‚Р°Р»
                                  СЃ low < curr.high.
              Р”Р»СЏ SELL (curr DOWN): stop = Р±Р»РёР¶Р°Р№С€РёР№ РїСЂРµРґС‹РґСѓС‰РёР№ UP-С„СЂР°РєС‚Р°Р»
                                    СЃ high > curr.low.
              Р•СЃР»Рё С‚Р°РєРѕРіРѕ РЅРµС‚ - СЃРёРіРЅР°Р» РїСЂРѕРїСѓСЃРєР°РµС‚СЃСЏ.
              Р Р°РЅСЊС€Рµ РїСЂРё prev UP < curr DOWN РїРѕР»СѓС‡Р°Р»СЃСЏ sl < entry РґР»СЏ SELL -
              Р»РѕР¶РЅС‹Р№ СЃС‚РѕРї РІРјРµСЃС‚Рѕ РїСЂРёР±С‹Р»Рё. РћС‚СЃСЋРґР° WR 24%.
- Р’Р»РёСЏРЅРёРµ:    РљРѕСЂСЂРµРєС‚РЅС‹Р№ SL. РћР¶РёРґР°РµРјС‹Р№ WR ~35-45%.
- РћС‚РєР°С‚:      Р’РµСЂРЅСѓС‚СЊ stop_line = prev.price.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0007 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/include/st/corridor.h,
              stpatterns/src/corridor.cpp,
              stpatterns/tools/st_emulator_main.cpp
- РўРёРї:        LOGIC
- РџСЂРёС‡РёРЅР°:    РЎРёРіРЅР°Р»С‹ С€Р»Рё РґСЂСѓРі РЅР° РґСЂСѓРіРµ. РџСЂР°РІРёР»Рѕ Р°РІС‚РѕСЂР° (СЃС‚СЂ.43):
              "С„СЂР°РєС‚Р°Р»С‹, РєРѕС‚РѕСЂС‹Рµ С†РµРЅР° РїРµСЂРµСЃРµРєР»Р° РґРѕ Р·Р°РІРµСЂС€РµРЅРёСЏ РїР°С‚С‚РµСЂРЅР°,
               РЅРµ РїРѕРґС…РѕРґСЏС‚ РґР»СЏ РїРѕСЃС‚СЂРѕРµРЅРёСЏ РЅРѕРІРѕР№ Р»РёРЅРёРё СЃС‚Р°СЂС‚Р°".
              Р”РѕР±Р°РІР»РµРЅР° РїСЂРѕСЃС‚Р°СЏ СЃРёРјСѓР»СЏС†РёСЏ: РѕС‚ entry РёРґС‘Рј РІРїРµСЂС‘Рґ РґРѕ SL РёР»Рё
              TP (4*height). РџРѕРєР° РїР°С‚С‚РµСЂРЅ РЅРµ Р·Р°РєСЂС‹Р»СЃСЏ - РЅРѕРІС‹Рµ СЃРёРіРЅР°Р»С‹
              РЅРµ Р±РµСЂС‘Рј.
              CorridorEvent СЂР°СЃС€РёСЂРµРЅ РїРѕР»СЏРјРё exit_idx, exit_price,
              exit_reason, pnl_pts.
- Р’Р»РёСЏРЅРёРµ:    РћР¶РёРґР°РµРјРѕРµ С‡РёСЃР»Рѕ СЃРґРµР»РѕРє ~2000-3000 Р·Р° 18 Р»РµС‚.
- РћС‚РєР°С‚:      РЈР±СЂР°С‚СЊ С„РёР»СЊС‚СЂ non-overlap, РѕСЃС‚Р°РІРёС‚СЊ dedup.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0006 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/src/corridor.cpp
- РўРёРї:        LOGIC
- РџСЂРёС‡РёРЅР°:    РЎРёРіРЅР°Р»С‹ РЅРµ Р±С‹Р»Рё РѕС‚СЃРѕСЂС‚РёСЂРѕРІР°РЅС‹ РїРѕ РІСЂРµРјРµРЅРё, СЃРѕРґРµСЂР¶Р°Р»Рё
              РґСѓР±Р»РёРєР°С‚С‹ РЅР° РѕРґРЅРѕРј Р±Р°СЂРµ. Р”РѕР±Р°РІР»РµРЅР° СЃРѕСЂС‚РёСЂРѕРІРєР° РїРѕ entry_idx
              Рё РґРµРґСѓРїР»РёРєР°С†РёСЏ (РѕРґРёРЅ СЃРёРіРЅР°Р» РЅР° Р±Р°СЂ, РїСЂРµРґРїРѕС‡С‚РµРЅРёРµ - РїРµСЂРІС‹Р№).
- Р’Р»РёСЏРЅРёРµ:    РћСЃРјС‹СЃР»РµРЅРЅР°СЏ СЃС‚Р°С‚РёСЃС‚РёРєР° СЃРёРіРЅР°Р»РѕРІ, РѕСЃРЅРѕРІР° РґР»СЏ СЌРјСѓР»СЏС†РёРё РїРѕР·РёС†РёРё.
- РћС‚РєР°С‚:      РЈР±СЂР°С‚СЊ sort + unique.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0005 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/src/corridor.cpp,
              stpatterns/tools/st_emulator_main.cpp
- РўРёРї:        LOGIC
- РџСЂРёС‡РёРЅР°:    (1) РџРѕСЂРѕРі РїСЂРѕР±РѕСЏ РїРµСЂРµРІРµРґС‘РЅ РІ 5-Р·РЅР°С‡РЅС‹Рµ РїСѓРЅРєС‚С‹:
              threshold_pts = 70.0 (= 7 pips РЅР° 5-digit РєРѕС‚РёСЂРѕРІРєР°С…).
              (2) РЈР±СЂР°РЅР° РіР»РѕР±Р°Р»СЊРЅР°СЏ Р±Р»РѕРєРёСЂРѕРІРєР° last_break_bar.
              РџСЂРёС‡РёРЅР°: РѕРЅР° РѕС‚Р±СЂР°СЃС‹РІР°Р»Р° РІСЃРµ СЃРёРіРЅР°Р»С‹ РїРѕСЃР»Рµ РїРµСЂРІРѕРіРѕ,
              С‚РµСЂСЏСЏ ~70% РёСЃС‚РѕСЂРёРё. Р’РјРµСЃС‚Рѕ РЅРµС‘ - Р»РѕРєР°Р»СЊРЅС‹Р№ РїСЂРѕРїСѓСЃРє:
              РєР°Р¶РґР°СЏ РїР°СЂР° РїСЂРѕС‚РёРІРѕРїРѕР»РѕР¶РЅС‹С… С„СЂР°РєС‚Р°Р»РѕРІ РёС‰РµС‚ РїСЂРѕР±РѕР№
              РЅРµР·Р°РІРёСЃРёРјРѕ.
- Р’Р»РёСЏРЅРёРµ:    РћР¶РёРґР°РµРјРѕРµ С‡РёСЃР»Рѕ СЃРёРіРЅР°Р»РѕРІ ~1500-2500 Р·Р° 18 Р»РµС‚.
- РћС‚РєР°С‚:      Р’РµСЂРЅСѓС‚СЊ threshold=7.0 Рё last_break_bar.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0004 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/include/st/corridor.h,
              stpatterns/src/corridor.cpp,
              stpatterns/tools/st_emulator_main.cpp,
              CMakeLists.txt
- РўРёРї:        MODULE
- РџСЂРёС‡РёРЅР°:    Р”РµС‚РµРєС‚РѕСЂ С„СЂР°РєС‚Р°Р»СЊРЅС‹С… РєРѕСЂРёРґРѕСЂРѕРІ + СЃРёРіРЅР°Р» РІС…РѕРґР°.
              Р›РѕРіРёРєР° Р°РІС‚РѕСЂР°:
              - РљРѕСЂРёРґРѕСЂ: РїР°СЂР° РїСЂРѕС‚РёРІРѕРїРѕР»РѕР¶РЅС‹С… С„СЂР°РєС‚Р°Р»РѕРІ, Р±Р»РёР¶Р°Р№С€Р°СЏ Рє Р±Р°СЂСѓ.
              - height_pts <= 0.5 * ADR(5) - РёРЅР°С‡Рµ РїСЂРѕРїСѓСЃРє.
              - РџСЂРѕР±РѕР№ start_line РЅР° threshold_pts (7) = СЃРёРіРЅР°Р».
              - РџРѕСЂРѕРі СѓС‡РёС‚С‹РІР°РµС‚ СЃРїСЂРµРґ (bar.spread).
              - РЈС‡РёС‚С‹РІР°РµРј С‚РѕР»СЊРєРѕ РїРѕР»РЅРѕСЃС‚СЊСЋ СЃС„РѕСЂРјРёСЂРѕРІР°РЅРЅС‹Рµ С„СЂР°РєС‚Р°Р»С‹.
- Р’Р»РёСЏРЅРёРµ:    Р’С…РѕРґ РІ СЃРґРµР»РєРё.
- РћС‚РєР°С‚:      РЈРґР°Р»РёС‚СЊ corridor.h/cpp.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0003 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/include/st/adr.h,
              stpatterns/src/adr.cpp,
              stpatterns/tools/st_emulator_main.cpp,
              CMakeLists.txt
- РўРёРї:        MODULE
- РџСЂРёС‡РёРЅР°:    ADR(5) - СЃСЂРµРґРЅРёР№ РґРЅРµРІРЅРѕР№ РґРёР°РїР°Р·РѕРЅ Р·Р° 5 РїСЂРµРґС‹РґСѓС‰РёС… РґРЅРµР№.
              Р¤РѕСЂРјСѓР»Р° Р°РІС‚РѕСЂР°:
              ADR(5) = sum(D1_Hi - D1_Li, i=1..5) / 5
              D1 Р±Р°СЂС‹ СЃС‚СЂРѕСЏС‚СЃСЏ РёР· H1 РіСЂСѓРїРїРёСЂРѕРІРєРѕР№ РїРѕ UTC РґР°С‚Рµ.
- Р’Р»РёСЏРЅРёРµ:    Р¤РёР»СЊС‚СЂ РєРѕСЂРёРґРѕСЂРѕРІ (corridor.height <= 0.5 * ADR).
- РћС‚РєР°С‚:      РЈРґР°Р»РёС‚СЊ adr.h/cpp, РѕС‚РєР°С‚РёС‚СЊ main.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0002 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/include/st/fractal.h,
              stpatterns/src/fractal.cpp,
              stpatterns/tools/st_emulator_main.cpp
- РўРёРї:        MODULE
- РџСЂРёС‡РёРЅР°:    Р”РµС‚РµРєС‚РѕСЂ 3-СЃРІРµС‡РЅС‹С… С„СЂР°РєС‚Р°Р»РѕРІ (Р°РІС‚РѕСЂ: "3 СЃРІРµС‡Рё Р»СѓС‡С€Рµ 5").
              Р’РµСЂС…РЅРёР№ С„СЂР°РєС‚Р°Р»: center.high >= left.high && center.high >= right.high
              РќРёР¶РЅРёР№ С„СЂР°РєС‚Р°Р»: center.low <= left.low && center.low <= right.low
              РџР»СЋСЃ СЃС‚СЂРѕРіРѕРµ РЅРµСЂР°РІРµРЅСЃС‚РІРѕ С…РѕС‚СЏ Р±С‹ СЃ РѕРґРЅРѕР№ СЃС‚РѕСЂРѕРЅС‹.
- Р’Р»РёСЏРЅРёРµ:    РџРµСЂРІС‹Р№ СЃР»РѕР№ STPatterns. РўРµСЃС‚ РЅР° EURUSD_H1.
- РћС‚РєР°С‚:      РЈРґР°Р»РёС‚СЊ fractal.h/cpp, РІРµСЂРЅСѓС‚СЊ main Р±РµР· С„СЂР°РєС‚Р°Р»РѕРІ.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0002 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/include/st/fractal.h,
              stpatterns/src/fractal.cpp,
              stpatterns/tools/st_emulator_main.cpp
- РўРёРї:        MODULE
- РџСЂРёС‡РёРЅР°:    Р”РµС‚РµРєС‚РѕСЂ 3-СЃРІРµС‡РЅС‹С… С„СЂР°РєС‚Р°Р»РѕРІ (Р°РІС‚РѕСЂ: "3 СЃРІРµС‡Рё Р»СѓС‡С€Рµ 5").
              Р’РµСЂС…РЅРёР№ С„СЂР°РєС‚Р°Р»: center.high >= left.high && center.high >= right.high
              РќРёР¶РЅРёР№ С„СЂР°РєС‚Р°Р»: center.low <= left.low && center.low <= right.low
              РџР»СЋСЃ СЃС‚СЂРѕРіРѕРµ РЅРµСЂР°РІРµРЅСЃС‚РІРѕ С…РѕС‚СЏ Р±С‹ СЃ РѕРґРЅРѕР№ СЃС‚РѕСЂРѕРЅС‹.
- Р’Р»РёСЏРЅРёРµ:    РџРµСЂРІС‹Р№ СЃР»РѕР№ STPatterns. РўРµСЃС‚ РЅР° EURUSD_H1.
- РћС‚РєР°С‚:      РЈРґР°Р»РёС‚СЊ fractal.h/cpp, РІРµСЂРЅСѓС‚СЊ main Р±РµР· С„СЂР°РєС‚Р°Р»РѕРІ.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE
## LIMB-0001 - 2026-09-30 - STP
- Р¤Р°Р№Р»С‹:      stpatterns/ (РІСЃС‘ РґРµСЂРµРІРѕ)
- РўРёРї:        CONTRACT
- РџСЂРёС‡РёРЅР°:    РЎРѕР·РґР°РЅРёРµ РїСЂРѕРµРєС‚Р° STPatterns. Р­РјСѓР»СЏС‚РѕСЂ СЃРёСЃС‚РµРјС‹ ST Patterns
              (РџРѕР»С‚РѕСЂР°С†РєРёР№, 2017) РЅР° C++. РСЃС‚РѕС‡РЅРёРє: РєРЅРёРіР° "ST Patterns
              of the Forex Рё С„СЊСЋС‡РµСЂСЃРЅС‹Рµ Р±РёСЂР¶Рё" (PDF).
- Р’Р»РёСЏРЅРёРµ:    РќРѕРІС‹Р№ РїСЂРѕРµРєС‚. РСЃРїРѕР»СЊР·СѓРµС‚ BarStream РёР· СЏРґСЂР° SPARTAK
              РґР»СЏ С‡С‚РµРЅРёСЏ XFBAR-С„Р°Р№Р»РѕРІ.
- РћС‚РєР°С‚:      РЈРґР°Р»РёС‚СЊ stpatterns/ Рё СѓРґР°Р»РёС‚СЊ target st_emulator РёР· CMakeLists.
- РЎС‚Р°С‚СѓСЃ:     ACTIVE