spartak_fx_research/
├─ CMakeLists.txt
├─ config/
│  ├─ cost_scenarios.conservative_v1.json
│  └─ research_config.locked_v1.txt
├─ include/spartak/
│  ├─ model.hpp                 # бары, сделки, спецификации
│  ├─ data.hpp                  # XFBAR001/002, M5, ресемплинг
│  ├─ conversion.hpp            # currency_profit → USD
│  ├─ costs.hpp                 # Base/Stress/Severe
│  ├─ strategy.hpp              # ИТ/ЗО/ОРТ/РМ state machine
│  ├─ backtest.hpp              # 50/25/25, Bid/Ask, gaps
│  ├─ metrics.hpp               # DSR, Calmar, drawdown
│  ├─ optimizer.hpp             # 12/72/648 grid
│  ├─ lock.hpp                  # SHA-256, design/selection locks
│  └─ runner.hpp                # четыре этапа исследования
├─ src/
│  ├─ data.cpp
│  ├─ conversion.cpp
│  ├─ costs.cpp
│  ├─ strategy.cpp
│  ├─ backtest.cpp
│  ├─ metrics.cpp
│  ├─ optimizer.cpp
│  ├─ lock.cpp
│  ├─ runner.cpp
│  └─ main.cpp
└─ tests/
   ├─ test_data.cpp
   ├─ test_costs.cpp
   ├─ test_strategy.cpp
   ├─ test_backtest.cpp
   ├─ test_metrics.cpp
   ├─ test_lock.cpp
   └─ test_runner.cpp