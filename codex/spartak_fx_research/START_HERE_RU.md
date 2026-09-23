# Начать здесь

Главный результат — `docs/project_report_ru.md`. В нём сведены правила системы, состояние данных, границы машинного baseline и причина, по которой blind OOS и top-50 пока не открывались.

Основные материалы:

- `docs/project_report_ru.md` — итоговый отчёт;
- `docs/trading_system_ru.md` — развёрнутая спецификация;
- `docs/video_timecode_evidence.md` — доказательные таймкоды;
- `docs/evaluation_protocol.md` — IS/validation/blind-OOS;
- `docs/data_format.md` — формат BIN;
- `docs/universe_inventory.csv` — 62 доступных M5-инструмента;
- `docs/release_verification.md` — сборка и 4/4 тестов;
- `config/research_config.example.json` — безопасная конфигурация с закрытым OOS;
- `include/`, `src/`, `tests/` — C++20-каркас;
- `bin/Release/` — две проверенные утилиты аудита данных и спецификаций;
- `transcripts/` и `contact_sheets/` — материалы всех 11 видео.

Статус: исследовательская инфраструктура готова; реальные комиссии/swap/funding, objective и криптографический experiment lock ещё должны быть зафиксированы. До этого top-50 не является допустимым результатом, а `allow_audit` остаётся `false`.
