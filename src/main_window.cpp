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
    setCentralWidget(canvas_ = new serigraph_widget(this));
    create_docks();
    create_menus();
    setWindowTitle(tr("serigraph"));
    resize(1200, 800);
}

ser::main_window::~main_window()
{
}

void ser::main_window::create_menus() {
    QMenu* file_menu = menuBar()->addMenu(tr("&File"));

    QAction* open_act = new QAction(tr("&Open..."), this);
    open_act->setShortcut(QKeySequence::Open);
    connect(open_act, &QAction::triggered, this, &main_window::open_file);
    file_menu->addAction(open_act);

    file_menu->addSeparator();

    QAction* exit_act = new QAction(tr("E&xit"), this);
    exit_act->setShortcut(QKeySequence::Quit);
    connect(exit_act, &QAction::triggered, this, &QWidget::close);
    file_menu->addAction(exit_act);

    // Optional: View menu to toggle docks
    QMenu* view_menu = menuBar()->addMenu(tr("&View"));
    view_menu->addAction(tr("Toggle Source Palette"), [this](bool) {
        if (auto* dw = findChild<QDockWidget*>("Source Dock"))
            dw->setVisible(!dw->isVisible());
        });
    view_menu->addAction(tr("Toggle Target Palette"), [this](bool) {
        if (auto* dw = findChild<QDockWidget*>("Target Dock"))
            dw->setVisible(!dw->isVisible());
        });
}

void  ser::main_window::add_color_to_palettes(const QColor& color) {
    source_palette_->add_color(color);
    target_palette_->add_color(color);
}

void ser::main_window::create_docks() {

    QDockWidget* source_dock = new QDockWidget(tr("Source Palette"), this);
    source_palette_ = new ser::palette_widget(source_dock);
    source_dock->setWidget(source_palette_);
    addDockWidget(Qt::LeftDockWidgetArea, source_dock);

    QDockWidget* target_dock = new QDockWidget(tr("Target Palette"), this);
    target_palette_ = new ser::palette_widget(target_dock);
    target_dock->setWidget(target_palette_);
    addDockWidget(Qt::RightDockWidgetArea, target_dock);

    source_palette_->set_colors({ Qt::white });
    target_palette_->set_colors({ Qt::white });

    connect(source_palette_, &ser::palette_widget::color_added_requested, this, &ser::main_window::add_color_to_palettes);

    connect(source_palette_, &ser::palette_widget::color_delete_requested, this, [this](int index) {
        source_palette_->remove_swatch_at(index);
        target_palette_->remove_swatch_at(index);
        });

    connect(source_palette_, &ser::palette_widget::palette_changed, this, [this]() {
        // m_canvas->update_source_palette(m_source_palette->get_colors());
        });

    connect(target_palette_, &ser::palette_widget::palette_changed, this, [this]() {
        // m_canvas->update_target_palette(m_target_palette->get_colors());
        });

    connect(canvas_, &ser::serigraph_widget::source_pixel_clicked,
        this, &ser::main_window::add_color_to_palettes);
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

        canvas_->set_source_image(image.convertToFormat(QImage::Format_RGB32));
    }
}