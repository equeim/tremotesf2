// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_NUMBERVALUERANGE_H
#define TREMOTESF_RPC_NUMBERVALUERANGE_H

#include <limits>

#include <QSpinBox>
#include <QDoubleSpinBox>

namespace tremotesf {
    template<typename T>
    struct NumberValueRange {
        T min;
        T max;
    };

    inline constexpr auto nonNegativeIntegersRange = NumberValueRange{.min = 0, .max = std::numeric_limits<int>::max()};

    inline constexpr auto nonNegativeDecimalsRange =
        NumberValueRange{.min = 0.0, .max = std::numeric_limits<double>::max()};

    // Transmission processes it as unsigned 16-bit integer
    inline constexpr auto minutesRange =
        NumberValueRange{.min = 0, .max = static_cast<int>(std::numeric_limits<unsigned short>::max())};
    inline constexpr auto portsRange = minutesRange;
    inline constexpr auto peersLimitRange = minutesRange;

    // Transmission processes it as unsigned 32-bit integer that's multiplied by 1000 and stored as unsigned 32-bit integer
    inline constexpr auto speedLimitKbpsRange =
        NumberValueRange{.min = 0, .max = static_cast<int>(std::numeric_limits<unsigned int>::max() / 1000)};

    inline void setSpinBoxLimits(QSpinBox* spinBox, NumberValueRange<int> range) {
        spinBox->setMinimum(range.min);
        spinBox->setMaximum(range.max);
    }

    inline void setSpinBoxLimits(QDoubleSpinBox* spinBox, NumberValueRange<double> range) {
        spinBox->setMinimum(range.min);
        spinBox->setMaximum(range.max);
    }
}

#endif // TREMOTESF_RPC_NUMBERVALUERANGE_H
