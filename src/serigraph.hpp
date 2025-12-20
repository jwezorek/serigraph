#include "color_lut.hpp"
#include "ink_layer.hpp"
#include <QImage>
#include <QColor>

namespace ser {

    ink_separation separate_image(const QImage& img, const color_lut& lut);
    QImage ink_layers_to_image(const ink_separation& layers, const std::vector<latent_space_color>& palette);

}