#pragma once

#include <QMainWindow>
#include "color_lut.hpp"
#include "ink_layer.hpp"

namespace ser{

    class serigraph_widget;
    class palette_widget;

    class main_window : public QMainWindow {
        Q_OBJECT

    public:
        main_window(QWidget* parent = nullptr);
        ~main_window();

    private slots:
        void open_file();

    private:
        void create_docks(); // New helper to setup the side panels
        void create_menus();
        void add_color_to_palettes(const QColor& color);
        void separate_layers();
        void reink();

        serigraph_widget* canvas_;
        ink_separation layers_;
        color_lut lut_;

        // New members for the palettes
        palette_widget* source_palette_;
        palette_widget* target_palette_;
    };
}