#include "ink_layer.hpp"

ser::ink_layer::ink_layer(int wd, int hgt) : impl_(wd * hgt, 0.0), wd_(wd), hgt_(hgt) {
}

size_t ser::ink_layer::index(int x, int y) const {
    return x + y * wd_;
}

double ser::ink_layer::operator()(int x, int y) const {
    return impl_[index(x, y)];
}

double& ser::ink_layer::operator()(int x, int y) {
    return impl_[index(x, y)];
}