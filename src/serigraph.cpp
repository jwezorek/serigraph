#include "serigraph.hpp"
#include <ranges>

namespace r = std::ranges;
namespace rv = std::ranges::views;

namespace {

    ser::ink_separation separate_image(const QImage& img, const ser::color_lut& lut) {
        int width = img.width();
        int height = img.height();

        // Determine the number of ink layers based on the palette size in the LUT
        // Each layer represents the coefficient k_i for a specific palette color[cite: 9, 36].
        size_t num_inks = lut.palette().size();
        ser::ink_separation layers;
        for (size_t i = 0; i < num_inks; ++i) {
            layers.emplace_back(width, height);
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                auto k = lut.look_up(img.pixel(x, y));
                for (size_t i = 0; i < num_inks; ++i) {
                    layers[i](x, y) = k[i];
                }
            }
        }

        return layers;
    }
}

std::tuple<ser::ink_separation, ser::color_lut> ser::separate_image(const QImage& img, const std::vector<QColor>& palette) {
    auto lut = color_lut( palette );
    auto sep = ::separate_image(img, lut);
    return { sep, lut };
}

QImage ser::ink_layers_to_image(const ink_separation& layers, const std::vector<latent_space_color>& palette) {
    if (layers.empty()) return QImage();

    int width = layers[0].width();  // Assuming accessors for ink_layer dimensions
    int height = layers[0].height();
    QImage result(width, height, QImage::Format_RGB32);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            coefficients k;
            k.reserve(layers.size());
            for (const auto& layer : layers) {
                k.push_back(layer(x, y));
            }

            auto color = color_from_ink_levels(k, palette);
            result.setPixelColor(x, y, color);
        }
    }

    return result;
}

QImage ser::ink_layers_to_image(const ink_separation& layers, const std::vector<QColor>& palette) {
    auto latent_space_palette = to_latent_space(palette);
    return ink_layers_to_image(layers, latent_space_palette);
}