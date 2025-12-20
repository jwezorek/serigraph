#include "serigraph.hpp"
#include <ranges>

namespace r = std::ranges;
namespace rv = std::ranges::views;

namespace {

    ser::rgb_color to_rgb(const QColor& color) {
        // Ensure we are working with RGB values (handles conversion if QColor was CMYK/HSL)
        QColor rgb = color.toRgb();

        // Return the std::array<uint8_t, 3> as defined in your color_lut
        return {
            static_cast<uint8_t>(rgb.red()),
            static_cast<uint8_t>(rgb.green()),
            static_cast<uint8_t>(rgb.blue())
        };
    }

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
                auto rgb = to_rgb(img.pixel(x, y));
                auto k = lut.look_up(rgb);
                for (size_t i = 0; i < num_inks; ++i) {
                    layers[i](x, y) = k[i];
                }
            }
        }

        return layers;
    }
}

std::tuple<ser::ink_separation, ser::color_lut> ser::separate_image(const QImage& img, const std::vector<QColor>& palette) {
    auto lut = color_lut( palette | rv::transform(::to_rgb) | r::to<std::vector>());
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

            rgb_color final_rgb = to_rgb(k, palette);
            result.setPixel(x, y, qRgb(final_rgb[0], final_rgb[1], final_rgb[2]));
        }
    }

    return result;
}