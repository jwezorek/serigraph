#include "main_window.h"
#include "serigraph_widget.h"
#include "palette_widget.hpp"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QScrollArea>
#include <QMessageBox>
#include <QDockWidget> 

ser::main_window::main_window(QWidget* parent)
    : QMainWindow(parent)
{
    setCentralWidget(m_canvas = new serigraph_widget(this));
    create_docks();

    setWindowTitle(tr("serigraph"));
    resize(1200, 800);
}

ser::main_window::~main_window()
{
}

void ser::main_window::create_docks() {

    QDockWidget* source_dock = new QDockWidget(tr("Source Palette"), this);
    m_source_palette = new ser::palette_widget(source_dock);
    source_dock->setWidget(m_source_palette);
    addDockWidget(Qt::LeftDockWidgetArea, source_dock);

    QDockWidget* target_dock = new QDockWidget(tr("Target Palette"), this);
    m_target_palette = new ser::palette_widget(target_dock);
    target_dock->setWidget(m_target_palette);
    addDockWidget(Qt::RightDockWidgetArea, target_dock);

    m_source_palette->set_colors({ Qt::white });
    m_target_palette->set_colors({ Qt::white });

    connect(m_source_palette, &ser::palette_widget::color_added_requested, this, [this](const QColor& color) {
        m_source_palette->add_color(color);
        m_target_palette->add_color(color);
        });

    connect(m_source_palette, &ser::palette_widget::color_delete_requested, this, [this](int index) {
        m_source_palette->remove_swatch_at(index);
        m_target_palette->remove_swatch_at(index);
        });

    connect(m_source_palette, &ser::palette_widget::palette_changed, this, [this]() {
        // m_canvas->update_source_palette(m_source_palette->get_colors());
        });

    connect(m_target_palette, &ser::palette_widget::palette_changed, this, [this]() {
        // m_canvas->update_target_palette(m_target_palette->get_colors());
        });
}

void ser::main_window::open_file() {
    QString file_name = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));

    if (!file_name.isEmpty()) {
        QImage image(file_name);
        if (image.isNull()) {
            QMessageBox::information(this, tr("Serigraph"),
                tr("Cannot load %1.").arg(file_name));
            return;
        }

        m_canvas->set_source_image(image.convertToFormat(QImage::Format_RGB32));
    }
}