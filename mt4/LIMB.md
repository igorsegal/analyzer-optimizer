# SPARTAK MT4 - LIMB

Change registry. Append-only.

## Format

## LIMB-NNNN - YYYY-MM-DDTHH:MM:SSZ - SPARTAK
- Files:      list
- Type:       CONTRACT | MODULE | LOGIC | CONFIG
- Reason:     text
- Impact:     what is affected
- Rollback:   how to revert
- Status:     ACTIVE | REVERTED | SUPERSEDED

## Exclusions (no record required)
- comments typos
- reformatting without semantics
- new modules not changing contracts

## LIMB-0001 - 2026-09-29T12:00:00Z - SPARTAK
- Files:      all
- Type:       CONTRACT
- Reason:     MT4 combine skeleton. Pure MQL4, no DLL.
- Impact:     All modules empty. Logic added one by one.
- Rollback:   Delete mt4 directory.
- Status:     ACTIVE