#include "serigraph.hpp"


ser::ink_separation ser::separate_image(const QImage& img, const color_lut& lut) {
    int width = img.width();
    int height = img.height();

    // Determine the number of ink layers based on the palette size in the LUT
    // Each layer represents the coefficient k_i for a specific palette color[cite: 9, 36].
    size_t num_inks = lut.palette().size();
    ink_separation layers;
    for (size_t i = 0; i < num_inks; ++i) {
        layers.emplace_back(width, height);
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 1. Extract the source RGB pixel
            QRgb pixel = img.pixel(x, y);
            rgb_color rgb = {
                static_cast<uint8_t>(qRed(pixel)),
                static_cast<uint8_t>(qGreen(pixel)),
                static_cast<uint8_t>(qBlue(pixel))
            };

            // 2. Perform trilinear interpolation in the 3D LUT to find 
            // the coefficient vector k[cite: 41].
            coefficients k = lut.look_up(rgb);

            // 3. Store each coefficient in its respective ink layer
            for (size_t i = 0; i < num_inks; ++i) {
                layers[i](x, y) = k[i];
            }
        }
    }

    return layers;
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