#pragma once

#include <vector>
#include <array>
#include <QColor>

class CoinPackedMatrix;

namespace ser {

    using coefficients = std::vector<double>;
    using latent_space_color = std::array<float, 7>;

    class color_lut {

        std::vector<std::vector<std::vector<coefficients>>> impl_;
        std::vector<latent_space_color> palette_;

        static coefficients solve_with_precomputed_q(
            const std::vector<latent_space_color>& palette,
            const ser::latent_space_color& target,
            const CoinPackedMatrix& Q);

    public:

        color_lut() {}

        color_lut(const std::vector<QColor>& palette);
        void reset_palette(const std::vector<QColor>& palette);
        coefficients look_up(const QColor& color) const;

        const std::vector<latent_space_color>& palette() const;
    };

    std::vector<latent_space_color> to_latent_space(const std::vector<QColor>& colors);
    QColor color_from_ink_levels(const coefficients& coeff, const std::vector<latent_space_color>& palette);

}