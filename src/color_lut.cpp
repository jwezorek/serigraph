#include "color_lut.hpp"
#include "third-party/mixbox.h"

// COIN-OR Clp Includes for Quadratic Programming
#include <ClpSimplex.hpp>
#include <CoinPackedMatrix.hpp>

#include <cmath>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

// -------------------------------------------------------------------------
// Internal Helpers & Constants (Anonymous Namespace)
// -------------------------------------------------------------------------
namespace {

    // Grid size for the 3D LUT (33x33x33)
    constexpr int LUT_GRID_SIZE = 33;

    // Regularization factor (lambda) for the Constrained Least Squares problem
    // Ensures stability ("Gray World") when exact matches are ambiguous
    constexpr double LAMBDA = 0.001;

    // Mixbox latent vector dimension (7D)
    constexpr int LATENT_DIM = MIXBOX_LATENT_SIZE;

    // Helper to clamp values
    template <typename T>
    T clamp(T val, T min, T max) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }

} // namespace


// -------------------------------------------------------------------------
// ser::color_lut Implementation
// -------------------------------------------------------------------------

ser::color_lut::color_lut(const std::vector<ser::rgb_color>& palette) {
    // Initialize the LUT structure: vector of vector of vector
    impl_.resize(LUT_GRID_SIZE);
    for (int i = 0; i < LUT_GRID_SIZE; ++i) {
        impl_[i].resize(LUT_GRID_SIZE);
        for (int j = 0; j < LUT_GRID_SIZE; ++j) {
            impl_[i][j].resize(LUT_GRID_SIZE);
        }
    }

    // Immediately "bake" the palette into the LUT
    reset_palette(palette);
}

void ser::color_lut::reset_palette(const std::vector<ser::rgb_color>& palette) {
    // 1. Convert Source Palette to Latent Space (Projection)
    palette_ = ser::to_latent_space(palette);

    // 2. Iterate through the lattice (Quantization)
    for (int r = 0; r < LUT_GRID_SIZE; ++r) {
        for (int g = 0; g < LUT_GRID_SIZE; ++g) {
            for (int b = 0; b < LUT_GRID_SIZE; ++b) {

                // Map grid index 0..32 to RGB 0..255
                uint8_t ur = static_cast<uint8_t>((r * 255) / (LUT_GRID_SIZE - 1));
                uint8_t ug = static_cast<uint8_t>((g * 255) / (LUT_GRID_SIZE - 1));
                uint8_t ub = static_cast<uint8_t>((b * 255) / (LUT_GRID_SIZE - 1));

                // Convert Lattice Point to Pigment Space (Mixbox)
                mixbox_latent latent_arr;
                mixbox_rgb_to_latent(ur, ug, ub, latent_arr);

                ser::latent_space_color target_color;
                std::copy(std::begin(latent_arr), std::end(latent_arr), target_color.begin());

                // 3. Solve QP for this node
                ser::coefficients coeffs = solve_for_color_coefficients(target_color);

                // 4. Store in LUT
                impl_[r][g][b] = coeffs;
            }
        }
    }
}

ser::coefficients ser::color_lut::solve_for_color_coefficients(const ser::latent_space_color& target) const {
    // Problem: Constrained Least Squares (Quadratic Programming)
    // Minimize || sum(k_i * V_i) - T ||^2 + lambda * sum(k_i^2)
    // Subject to: sum(k_i) = 1.0, 0 <= k_i <= 1.0

    // QP Standard Form: minimize 0.5 * k'Qk + c'k
    // Expansion of Least Squares gives:
    // Q = 2 * (V'V + lambda*I)
    // c = -2 * V'T

    int n_colors_int = static_cast<int>(palette_.size());
    if (n_colors_int == 0) return {};

    // --- Step A: Construct Vector c (Linear Term) ---
    std::vector<double> c_vec(n_colors_int, 0.0);

    for (int i = 0; i < n_colors_int; ++i) {
        double dot_vt = 0.0;
        for (int d = 0; d < LATENT_DIM; ++d) {
            dot_vt += palette_[i][d] * target[d];
        }
        c_vec[i] = -2.0 * dot_vt;
    }

    // --- Step B: Setup ClpSimplex Solver ---
    ClpSimplex model;
    model.setLogLevel(0); // Silent

    // 1. Add Columns (Variables k_i) with Non-negativity constraints 0..1
    for (int i = 0; i < n_colors_int; ++i) {
        model.addColumn(0, nullptr, nullptr, 0.0, 1.0, c_vec[i]);
    }

    // 2. Add Convexity Constraint: Sum(k_i) = 1.0
    std::vector<int> col_indices_row(n_colors_int);
    std::vector<double> row_elements(n_colors_int, 1.0);
    std::iota(col_indices_row.begin(), col_indices_row.end(), 0);

    model.addRow(n_colors_int, col_indices_row.data(), row_elements.data(), 1.0, 1.0);

    // --- Step C: Construct Sparse Matrix Q (Quadratic Term) ---
    // We must build Q in Compressed Sparse Column (CSC) format manually.
    // Q = 2 * (V'V + lambda*I)

    // IMPORTANT: Types must strictly match CoinPackedMatrix.hpp definitions
    std::vector<CoinBigIndex> col_starts; // Must be CoinBigIndex*
    std::vector<int> row_indices;         // Must be int*
    std::vector<double> elements;         // Must be double*
    std::vector<int> col_lengths;         // Must be int*

    col_starts.reserve(n_colors_int + 1);
    col_lengths.reserve(n_colors_int);
    row_indices.reserve(n_colors_int * n_colors_int);
    elements.reserve(n_colors_int * n_colors_int);

    CoinBigIndex current_idx = 0;

    for (int j = 0; j < n_colors_int; ++j) { // Columns
        col_starts.push_back(current_idx);
        int len = 0;

        for (int i = 0; i < n_colors_int; ++i) { // Rows
            // Calculate Q[i][j]
            double dot = 0.0;
            for (int d = 0; d < LATENT_DIM; ++d) {
                dot += palette_[i][d] * palette_[j][d];
            }

            // Add regularization to diagonal
            if (i == j) {
                dot += LAMBDA;
            }

            // Scale by 2.0 for QP form
            double val = 2.0 * dot;

            // Add to sparse matrix if non-zero
            if (std::abs(val) > 1e-12) {
                row_indices.push_back(i);
                elements.push_back(val);
                len++;
                current_idx++;
            }
        }
        col_lengths.push_back(len);
    }
    col_starts.push_back(current_idx); // Final start index

    // Create CoinPackedMatrix using the 8-argument constructor found in header
    // Signature: (bool colordered, int minor, int major, CoinBigIndex numels, 
    //             double *elem, int *ind, CoinBigIndex *start, int *len)
    CoinPackedMatrix coin_Q(true,
        n_colors_int,
        n_colors_int,
        current_idx,         // numels
        elements.data(),
        row_indices.data(),
        col_starts.data(),
        col_lengths.data());

    // Load into solver
    model.loadQuadraticObjective(coin_Q);

    // --- Step D: Solve ---
    model.primal();

    // --- Step E: Extract Results ---
    double* solution = model.primalColumnSolution();

    ser::coefficients result(n_colors_int);
    for (int i = 0; i < n_colors_int; ++i) {
        result[i] = clamp(solution[i], 0.0, 1.0);
    }

    return result;
}

ser::coefficients ser::color_lut::look_up(const ser::rgb_color& color) const {
    // Sample the coefficient vector using Trilinear Interpolation

    // Normalize 0-255 to Grid Coordinates 0-32
    float r_pos = color[0] * (LUT_GRID_SIZE - 1) / 255.0f;
    float g_pos = color[1] * (LUT_GRID_SIZE - 1) / 255.0f;
    float b_pos = color[2] * (LUT_GRID_SIZE - 1) / 255.0f;

    // Integer parts
    int r0 = clamp(static_cast<int>(r_pos), 0, LUT_GRID_SIZE - 2);
    int g0 = clamp(static_cast<int>(g_pos), 0, LUT_GRID_SIZE - 2);
    int b0 = clamp(static_cast<int>(b_pos), 0, LUT_GRID_SIZE - 2);

    int r1 = r0 + 1;
    int g1 = g0 + 1;
    int b1 = b0 + 1;

    // Fractional parts
    float tr = r_pos - r0;
    float tg = g_pos - g0;
    float tb = b_pos - b0;

    // Fetch coefficients for the 8 corners of the lattice cell
    const auto& c000 = impl_[r0][g0][b0];
    const auto& c100 = impl_[r1][g0][b0];
    const auto& c010 = impl_[r0][g1][b0];
    const auto& c110 = impl_[r1][g1][b0];
    const auto& c001 = impl_[r0][g0][b1];
    const auto& c101 = impl_[r1][g0][b1];
    const auto& c011 = impl_[r0][g1][b1];
    const auto& c111 = impl_[r1][g1][b1];

    size_t n_coeffs = c000.size();
    ser::coefficients result(n_coeffs);

    for (size_t i = 0; i < n_coeffs; ++i) {
        // Interpolate R
        float c00 = c000[i] * (1 - tr) + c100[i] * tr;
        float c01 = c001[i] * (1 - tr) + c101[i] * tr;
        float c10 = c010[i] * (1 - tr) + c110[i] * tr;
        float c11 = c011[i] * (1 - tr) + c111[i] * tr;

        // Interpolate G
        float c0 = c00 * (1 - tg) + c10 * tg;
        float c1 = c01 * (1 - tg) + c11 * tg;

        // Interpolate B
        result[i] = c0 * (1 - tb) + c1 * tb;
    }

    return result;
}

const std::vector<ser::latent_space_color>& ser::color_lut::palette() const {
    return palette_;
}

// -------------------------------------------------------------------------
// ser:: Free Functions
// -------------------------------------------------------------------------

std::vector<ser::latent_space_color> ser::to_latent_space(const std::vector<ser::rgb_color>& colors) {
    std::vector<ser::latent_space_color> result;
    result.reserve(colors.size());

    for (const auto& rgb : colors) {
        mixbox_latent latent_arr;
        // Convert RGB -> Pigment Latent Space
        mixbox_rgb_to_latent(rgb[0], rgb[1], rgb[2], latent_arr);

        ser::latent_space_color lsc;
        std::copy(std::begin(latent_arr), std::end(latent_arr), lsc.begin());
        result.push_back(lsc);
    }
    return result;
}

ser::rgb_color ser::to_rgb(const ser::coefficients& coeff, const std::vector<ser::latent_space_color>& palette) {
    if (coeff.empty() || palette.empty() || coeff.size() != palette.size()) {
        return { 0, 0, 0 };
    }

    // Perform linear combination in Pigment Space
    mixbox_latent mixed_latent = { 0 };

    for (size_t i = 0; i < coeff.size(); ++i) {
        float weight = static_cast<float>(coeff[i]);
        for (int d = 0; d < LATENT_DIM; ++d) {
            mixed_latent[d] += weight * palette[i][d];
        }
    }

    // Convert Pigment Space back to RGB
    unsigned char r, g, b;
    mixbox_latent_to_rgb(mixed_latent, &r, &g, &b);

    return { r, g, b };
}