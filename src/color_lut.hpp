#pragma once

#include <vector>
#include <array>
#include <QColor>

namespace ser {

    using coefficients = std::vector<double>;
    using latent_space_color = std::array<float, 7>;

    class color_lut {

        std::vector<std::vector<std::vector<coefficients>>> impl_;
        std::vector<latent_space_color> palette_;

        // use CBC to solve the QP problem separating 'color' into percentages of palette colors...
        static coefficients solve_for_color_coefficients(
            const std::vector<latent_space_color>& palette, const latent_space_color& color
        );

    public:

        color_lut() {}

        // just call reset reset_palette below...
        color_lut(const std::vector<QColor>& palette);

        // convert the palette to latent space, store in palette_ and
        // create the 33x33x33 lookup table called impl_
        void reset_palette(const std::vector<QColor>& palette);

        // interpolate between values in impl_...
        coefficients look_up(const QColor& color) const;

        const std::vector<latent_space_color>& palette() const;
    };

    std::vector<latent_space_color> to_latent_space(const std::vector<QColor>& colors);
    QColor color_from_ink_levels(const coefficients& coeff, const std::vector<latent_space_color>& palette);

}