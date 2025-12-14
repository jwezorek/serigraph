#pragma once

#include <QMainWindow>

class serigraph_widget;

// Forward declaration for the palette widget
namespace ser {
    class palette_widget;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window(QWidget* parent = nullptr);
    ~main_window();

private slots:
    void open_file();

private:
    void create_actions();
    void create_menus();
    void create_docks(); // New helper to setup the side panels

    serigraph_widget* m_canvas;

    // New members for the palettes
    ser::palette_widget* m_source_palette;
    ser::palette_widget* m_target_palette;
};