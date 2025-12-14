# Serigraph

Serigraph is a novel image processing engine designed to decompose digital images into "ink layers" based on a user-defined palette. Unlike standard filters that map colors based on luminance (Gradient Maps) or global shifts (LUTs), Serigraph calculates the geometric relationship between image pixels and palette colors in a high-dimensional pigment space.

The result is a "semantic recoloring" tool that mimics the physics of screen printing: preserving the texture and identity of objects while allowing the user to swap the material properties (inks) of the scene.

## Mathematical Formulation

The core of Serigraph is a **Constrained Least Squares** problem solved via Quadratic Programming.

### The Objective
For every target color $T$ in the source image and a set of palette colors $\{V_1, ..., V_n\}$, the engine seeks a coefficient vector $\mathbf{k}$ to minimize the squared Euclidean distance between the target and the reconstructed color.

$$\text{minimize} \quad || (\sum_{i=1}^{n} k_i V_i) - T ||_2^2 + \lambda \sum_{i=1}^{n} k_i^2$$

### Constraints
The solution is subject to strict physical constraints:
* **Non-negativity:** $0 \le k_i \le 1.0$ (Negative ink is impossible).
* **Convexity:** $\sum_{i=1}^{n} k_i = 1.0$ (The mixture must fully cover the pixel).

### Regularization
The regularization term ($\lambda \sum k_i^2$) is critical for temporal stability. Without it, the solver may return sparse, unstable solutions (e.g., flipping between 100% Color A and 100% Color B) for colors lying between two palette nodes. The L2 term forces the distribution of weights across multiple palette colors when a perfect match isn't possible, ensuring smooth gradients.

## Pigment Latent Space

Standard decomposition algorithms operate in RGB space, where Red + Green = Yellow. Serigraph innovates by performing all mathematical operations in **Pigment Latent Space** using the Kubelka-Munk model.

* **Spectral Vectors:** RGB values are converted into 7-dimensional vectors representing absorption and scattering coefficients.
* **Physical Mixing:** The linear sum $\sum k_i V_i$ simulates physical paint mixing. For example, Blue + Yellow results in Green (not Gray), and Deep Red + Deep Blue results in Black/Purple (not Magenta).

## Technical Architecture

Solving a Quadratic Program for every pixel in a 4K image is not feasible in real-time. Serigraph employs a two-phase **3D LUT (Look-Up Table)** strategy.

### Phase A: The "Baking" (Heavy Compute)
Occurs when an image is loaded or the Source Palette changes.
1.  **Quantization:** The color space is discretized into a lattice (e.g., $33 \times 33 \times 33$).
2.  **Projection:** Lattice points and palette colors are projected into Pigment Space.
3.  **Solver Execution:** The system uses **Clp (COIN-OR Linear Programming)** to solve the QP for every node in the lattice.
4.  **Storage:** The resulting coefficient vectors $\mathbf{k}$ are stored in the 3D LUT.

### Phase B: The "Rendering" (Real-Time)
Occurs when the user interacts with the GUI to modify the Destination Palette.
1.  **Pixel Shader:** For every pixel, the engine samples the coefficient vector $\mathbf{k}$ from the 3D LUT using Trilinear Interpolation.
2.  **Reconstruction:** The new color is computed instantaneously: $C_{new} = \sum_{i=1}^{n} k_i \cdot P_{dst, i}$.

## Implementation Stack

* **Language:** Modern C++ (C++23).
* **Solver:** **COIN-OR Clp** (Quadratic Programming interface). Chosen for its efficiency with sum=1 and 0..1 constraints.
* **Color Library:** **Mixbox** (or custom Kubelka-Munk implementation) for RGB $\leftrightarrow$ Latent Space conversion.
* **Rendering:** OpenGL/Vulkan for the 3D LUT lookup and final render pass.

## Aesthetic Properties

* **Luminance Unlocking:** Unlike gradient maps, dark source pixels can become bright output pixels if mapped to a bright palette color.
* **Gamut Clipping:** Highly saturated source colors not present in the Source Palette are "pulled" toward the convex hull of the palette, creating natural desaturation rather than artifacts.
* **Physical Consistency:** Gradients behave like ink on paper; changing a single palette color effects the mix of surrounding pixels organically.
