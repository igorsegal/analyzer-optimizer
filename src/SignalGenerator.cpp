#include "../include/SignalGenerator.h"
#include <cmath>
#include <algorithm>

namespace AHexa {

Signal SignalGenerator::generate(const Bar& currentBar,
                                 const std::vector<Level>& levels,
                                 Trend trend,
                                 double mlProb,
                                 const StrategyParams& params) {
    Signal sig;
    sig.direction = Signal::Direction::NONE;
    sig.entryPrice = 0.0;
    sig.stopLoss = 0.0;
    sig.takeProfit = 0.0;

    // 1. Проверяем ML-порог
    if (mlProb < params.mlThreshold) return sig;

    // 2. Ищем ближайший уровень
    const Level* nearestLevel = nullptr;
    double minDist = 1e9;
    for (const auto& lvl : levels) {
        double dist = std::abs(currentBar.close - lvl.price);
        if (dist < minDist) {
            minDist = dist;
            nearestLevel = &lvl;
        }
    }
    if (!nearestLevel) return sig;

    // 3. Проверяем, что цена в зоне уровня (допуск в пунктах)
    double tolerancePrice = params.levelTolerancePips * 0.0001; // грубо
    if (minDist > tolerancePrice) return sig;

    // 4. Определяем направление по тренду и типу уровня
    // Для BUY: уровень поддержки + тренд BULL или SIDEWAYS
    // Для SELL: уровень сопротивления + тренд BEAR или SIDEWAYS
    bool canBuy = (nearestLevel->type == Level::Type::SUPPORT) &&
                  (trend == Trend::BULL || trend == Trend::SIDEWAYS);
    bool canSell = (nearestLevel->type == Level::Type::RESISTANCE) &&
                   (trend == Trend::BEAR || trend == Trend::SIDEWAYS);

    if (!canBuy && !canSell) return sig;

    // 5. Формируем сигнал
    double point = 0.0001; // упрощённо (можно брать из заголовка)
    if (canBuy) {
        sig.direction = Signal::Direction::BUY;
        sig.entryPrice = currentBar.close;
        sig.stopLoss = nearestLevel->price - params.levelTolerancePips * point;
        sig.takeProfit = sig.entryPrice + (sig.entryPrice - sig.stopLoss) * params.takeProfitRatio;
    } else if (canSell) {
        sig.direction = Signal::Direction::SELL;
        sig.entryPrice = currentBar.close;
        sig.stopLoss = nearestLevel->price + params.levelTolerancePips * point;
        sig.takeProfit = sig.entryPrice - (sig.stopLoss - sig.entryPrice) * params.takeProfitRatio;
    }

    return sig;
}

} // namespace AHexa