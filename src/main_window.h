#pragma once

#include <QMainWindow>

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

        serigraph_widget* m_canvas;

        // New members for the palettes
        palette_widget* m_source_palette;
        palette_widget* m_target_palette;
    };
}