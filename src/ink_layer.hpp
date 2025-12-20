#pragma once

#include <vector>

namespace ser {

    class ink_layer {
    
        std::vector<double> impl_;
        int wd_;
        int hgt_;

        size_t index(int x, int y) const;
    
    public:

        ink_layer(int wd, int hgt);
        double operator()(int x, int y) const;
        double& operator()(int x, int y);
        int width() const;
        int height() const;
    };

    using ink_separation = std::vector<ink_layer>;

}