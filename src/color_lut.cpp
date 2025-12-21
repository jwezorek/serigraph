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
#include <execution> // Required for std::execution::par
#include <qdebug.h>
#include <qcolor.h>

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

ser::color_lut::color_lut(const std::vector<QColor>& palette) {
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

void ser::color_lut::reset_palette(const std::vector<QColor>& palette) {
    // 1. Convert Source Palette to Latent Space
    palette_ = ser::to_latent_space(palette);
    int n_colors = static_cast<int>(palette_.size());
    if (n_colors == 0) return;

    // 2. Pre-calculate the Hessian (Q Matrix)
    // Q = 2 * (V'V + lambda*I). Clp requires Lower Triangular matrix for Barrier.
    std::vector<CoinBigIndex> col_starts;
    std::vector<int> row_indices;
    std::vector<double> elements;
    std::vector<int> col_lengths;

    col_starts.reserve(n_colors + 1);
    col_lengths.reserve(n_colors);
    row_indices.reserve((n_colors * (n_colors + 1)) / 2);
    elements.reserve((n_colors * (n_colors + 1)) / 2);

    CoinBigIndex current_idx = 0;
    for (int j = 0; j < n_colors; ++j) {
        col_starts.push_back(current_idx);
        int len = 0;
        for (int i = j; i < n_colors; ++i) {
            double dot = 0.0;
            for (int d = 0; d < LATENT_DIM; ++d) {
                dot += palette_[i][d] * palette_[j][d];
            }
            if (i == j) dot += LAMBDA;

            double val = 2.0 * dot;
            if (std::abs(val) > 1e-12) {
                row_indices.push_back(i);
                elements.push_back(val);
                len++;
                current_idx++;
            }
        }
        col_lengths.push_back(len);
    }
    col_starts.push_back(current_idx);

    CoinPackedMatrix shared_Q(true, n_colors, n_colors, current_idx,
        elements.data(), row_indices.data(),
        col_starts.data(), col_lengths.data());

    // 3. Prepare Parallel Loop (using flat index range)
    const int total_cells = LUT_GRID_SIZE * LUT_GRID_SIZE * LUT_GRID_SIZE;
    std::vector<int> indices(total_cells);
    std::iota(indices.begin(), indices.end(), 0);

    // 4. Parallel Solve using C++17 Execution Policy
    std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int idx) {
        // Map 1D index back to 3D grid
        int r = idx / (LUT_GRID_SIZE * LUT_GRID_SIZE);
        int g = (idx / LUT_GRID_SIZE) % LUT_GRID_SIZE;
        int b = idx % LUT_GRID_SIZE;

        // Map grid index to RGB 0..255
        uint8_t ur = static_cast<uint8_t>((r * 255) / (LUT_GRID_SIZE - 1));
        uint8_t ug = static_cast<uint8_t>((g * 255) / (LUT_GRID_SIZE - 1));
        uint8_t ub = static_cast<uint8_t>((b * 255) / (LUT_GRID_SIZE - 1));

        mixbox_latent latent_arr;
        mixbox_rgb_to_latent(ur, ug, ub, latent_arr);
        ser::latent_space_color target_color;
        std::copy(std::begin(latent_arr), std::end(latent_arr), target_color.begin());

        // Solve for this node (each thread creates its own Clp instance)
        impl_[r][g][b] = solve_with_precomputed_q(palette_, target_color, shared_Q);
        });
}

ser::coefficients ser::color_lut::solve_with_precomputed_q(
    const std::vector<ser::latent_space_color>& palette,
    const ser::latent_space_color& target,
    const CoinPackedMatrix& Q) {

    int n_colors = static_cast<int>(palette.size());

    // Setup local solver instance (thread-safe construction)
    ClpSimplex model;
    model.setLogLevel(0);

    // Add variables (k_i) with constraints 0.0 <= k_i <= 1.0
    // Objective linear term: c = -2 * V'T
    for (int i = 0; i < n_colors; ++i) {
        double dot_vt = 0.0;
        for (int d = 0; d < LATENT_DIM; ++d) {
            dot_vt += palette[i][d] * target[d];
        }
        model.addColumn(0, nullptr, nullptr, 0.0, 1.0, -2.0 * dot_vt);
    }

    // Add Convexity Constraint: Sum(k_i) = 1.0
    std::vector<int> col_indices(n_colors);
    std::vector<double> row_elements(n_colors, 1.0);
    std::iota(col_indices.begin(), col_indices.end(), 0);
    model.addRow(n_colors, col_indices.data(), row_elements.data(), 1.0, 1.0);

    // Load precomputed Q (Hessian) and solve using Barrier (required for QP)
    model.loadQuadraticObjective(Q);
    model.barrier(false);

    double* solution = model.primalColumnSolution();
    ser::coefficients result(n_colors);

    if (solution) {
        for (int i = 0; i < n_colors; ++i) {
            result[i] = clamp(solution[i], 0.0, 1.0);
        }
    } else {
        result.assign(n_colors, 0.0);
    }

    return result;
}

ser::coefficients ser::color_lut::look_up(const QColor& color) const {
    // Sample the coefficient vector using Trilinear Interpolation
    float r_pos = color.red() * (LUT_GRID_SIZE - 1) / 255.0f;
    float g_pos = color.green() * (LUT_GRID_SIZE - 1) / 255.0f;
    float b_pos = color.blue() * (LUT_GRID_SIZE - 1) / 255.0f;

    int r0 = clamp(static_cast<int>(r_pos), 0, LUT_GRID_SIZE - 2);
    int g0 = clamp(static_cast<int>(g_pos), 0, LUT_GRID_SIZE - 2);
    int b0 = clamp(static_cast<int>(b_pos), 0, LUT_GRID_SIZE - 2);

    int r1 = r0 + 1;
    int g1 = g0 + 1;
    int b1 = b0 + 1;

    float tr = r_pos - r0;
    float tg = g_pos - g0;
    float tb = b_pos - b0;

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
        float c00 = c000[i] * (1 - tr) + c100[i] * tr;
        float c01 = c001[i] * (1 - tr) + c101[i] * tr;
        float c10 = c010[i] * (1 - tr) + c110[i] * tr;
        float c11 = c011[i] * (1 - tr) + c111[i] * tr;

        float c0 = c00 * (1 - tg) + c10 * tg;
        float c1 = c01 * (1 - tg) + c11 * tg;

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

std::vector<ser::latent_space_color> ser::to_latent_space(const std::vector<QColor>& colors) {
    std::vector<ser::latent_space_color> result;
    result.reserve(colors.size());

    for (const auto& rgb : colors) {
        mixbox_latent latent_arr;
        mixbox_rgb_to_latent(rgb.red(), rgb.green(), rgb.blue(), latent_arr);

        ser::latent_space_color lsc;
        std::copy(std::begin(latent_arr), std::end(latent_arr), lsc.begin());
        result.push_back(lsc);
    }
    return result;
}

QColor ser::color_from_ink_levels(const ser::coefficients& coeff, const std::vector<ser::latent_space_color>& palette) {
    if (coeff.empty() || palette.empty() || coeff.size() != palette.size()) {
        return { 0, 0, 0 };
    }

    mixbox_latent mixed_latent = { 0 };
    for (size_t i = 0; i < coeff.size(); ++i) {
        float weight = static_cast<float>(coeff[i]);
        for (int d = 0; d < LATENT_DIM; ++d) {
            mixed_latent[d] += weight * palette[i][d];
        }
    }

    unsigned char r, g, b;
    mixbox_latent_to_rgb(mixed_latent, &r, &g, &b);
    return { r, g, b };
}