#include "color_lut.hpp"
#include "ink_layer.hpp"
#include <QImage>
#include <QColor>
#include <tuple>

namespace ser {

    std::tuple<ink_separation, color_lut> separate_image(const QImage& img, const std::vector<QColor>& palette);
    QImage ink_layers_to_image(const ink_separation& layers, const std::vector<latent_space_color>& palette);
    QImage ink_layers_to_image(const ink_separation& layers, const std::vector<QColor>& palette);

}