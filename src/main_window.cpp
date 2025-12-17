#include "main_window.h"
#include "serigraph_widget.h"
#include "palette_widget.hpp" // Include the new widget header

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QScrollArea>
#include <QMessageBox>
#include <QDockWidget> // Required for docking

main_window::main_window(QWidget* parent)
    : QMainWindow(parent)
{

    setCentralWidget(m_canvas = new serigraph_widget(this));

    // 3. UI Setup
    create_actions();
    create_menus();
    create_docks(); // Setup the palette widgets

    setWindowTitle(tr("Serigraph"));
    resize(1200, 800); // Increased size slightly to accommodate docks
}

main_window::~main_window() {}

void main_window::create_actions() {
    // Actions are created directly in create_menus for brevity
}

void main_window::create_menus() {
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

void main_window::create_docks() {
    // --- Source Palette (Input) ---
    QDockWidget* source_dock = new QDockWidget(tr("Source Palette"), this);
    source_dock->setObjectName("Source Dock");
    source_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_source_palette = new ser::palette_widget(source_dock);

    // Initialize with some default "Input" colors (e.g., standard CMYK + RGB + BW)
    m_source_palette->set_colors({
        Qt::black, Qt::white, Qt::red, Qt::green, Qt::blue,
        Qt::cyan, Qt::magenta, Qt::yellow
        });

    source_dock->setWidget(m_source_palette);
    addDockWidget(Qt::LeftDockWidgetArea, source_dock);

    // --- Target Palette (Output) ---
    QDockWidget* target_dock = new QDockWidget(tr("Target Palette"), this);
    target_dock->setObjectName("Target Dock");
    target_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_target_palette = new ser::palette_widget(target_dock);

    // Initialize with some "Artistic" default colors
    m_target_palette->set_colors({
        QColor(40, 40, 45),    // Charcoal
        QColor(230, 220, 210), // Off-white paper
        QColor(200, 60, 60),   // Faded Red
        QColor(60, 100, 160),  // Denim Blue
        QColor(220, 180, 60)   // Mustard
        });

    target_dock->setWidget(m_target_palette);
    addDockWidget(Qt::RightDockWidgetArea, target_dock);

    // --- Connections (Placeholders for now) ---
    // When palettes change, we will eventually tell the canvas to re-bake or re-render
    connect(m_source_palette, &ser::palette_widget::palette_changed, this, [this]() {
        // TODO: m_canvas->set_source_palette(m_source_palette->get_colors());
        });

    connect(m_target_palette, &ser::palette_widget::palette_changed, this, [this]() {
        // TODO: m_canvas->set_target_palette(m_target_palette->get_colors());
        });
}

void main_window::open_file() {
    QString file_name = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));

    if (!file_name.isEmpty()) {
        QImage image(file_name);
        if (image.isNull()) {
            QMessageBox::information(this, tr("Serigraph"),
                tr("Cannot load %1.").arg(file_name));
            return;
        }

        // Convert to RGB32 for consistent memory access in your future solver
        m_canvas->set_source_image(image.convertToFormat(QImage::Format_RGB32));
    }
}