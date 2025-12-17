#include "serigraph_widget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QDebug>

// --- Internal Helper Class: ImagePane ---
class ImagePane : public QWidget {
    Q_OBJECT
public:
    explicit ImagePane(QWidget* parent = nullptr) : QWidget(parent) {
        // Default policy
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    void set_image(const QImage& img) {
        m_image = img;
        // We do NOT call update() here immediately; 
        // the parent widget handles resizing which triggers the repaint.
    }

    const QImage& image() const { return m_image; }

signals:
    void pixel_clicked(QColor color);

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);

        if (m_image.isNull()) {
            // Draw placeholder background
            painter.fillRect(rect(), QColor(50, 50, 50)); // Dark grey
            painter.setPen(Qt::lightGray);
            painter.drawText(rect(), Qt::AlignCenter, "[ No Image Loaded ]");
        }
        else {
            // Draw image
            painter.drawImage(0, 0, m_image);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (m_image.isNull()) return;
        QPoint pos = event->pos();
        if (m_image.valid(pos)) {
            emit pixel_clicked(m_image.pixelColor(pos));
        }
    }

private:
    QImage m_image;
};

#include "serigraph_widget.moc"

// --- Main Widget Implementation ---

ser::serigraph_widget::serigraph_widget(QWidget* parent)
    : QTabWidget(parent)
{
    // Place tabs at the bottom
    setTabPosition(QTabWidget::South);

    // Create the three tabs with embedded scroll areas
    m_source_pane = create_tab("Source", m_source_scroll);
    m_separated_pane = create_tab("Separated", m_separated_scroll);
    m_reinked_pane = create_tab("Re-inked", m_reinked_scroll);

    // Connect the source click signal
    connect(m_source_pane, &ImagePane::pixel_clicked,
        this, &serigraph_widget::source_pixel_clicked);
}

ImagePane* ser::serigraph_widget::create_tab(const QString& title, QScrollArea*& out_scroll_area) {
    // 1. Create the Scroll Area
    out_scroll_area = new QScrollArea(this);
    out_scroll_area->setBackgroundRole(QPalette::Dark);
    out_scroll_area->setAlignment(Qt::AlignCenter);

    // 2. Create the Image Pane
    ImagePane* pane = new ImagePane(out_scroll_area);

    // 3. Setup relationship
    out_scroll_area->setWidget(pane);

    // Initially resizable = true so the "Empty" text centers in the full window
    out_scroll_area->setWidgetResizable(true);

    // 4. Add to TabWidget
    addTab(out_scroll_area, title);

    return pane;
}

void ser::serigraph_widget::update_scroll_behavior(ImagePane* pane, QScrollArea* scroll, const QImage& img) {
    pane->set_image(img);

    if (img.isNull()) {
        // If empty, let the widget expand to fill the scroll area so the text centers
        scroll->setWidgetResizable(true);
    }
    else {
        // If image exists, snap widget to image size and let scrollbars handle the rest
        scroll->setWidgetResizable(false);
        pane->resize(img.size());
    }
    pane->update();
}

void ser::serigraph_widget::set_source_image(const QImage& image) {
    update_scroll_behavior(m_source_pane, m_source_scroll, image);
    setCurrentWidget(m_source_scroll); // Auto-focus this tab
}

void ser::serigraph_widget::set_separated_image(const QImage& image) {
    update_scroll_behavior(m_separated_pane, m_separated_scroll, image);
}

void ser::serigraph_widget::set_reinked_image(const QImage& image) {
    update_scroll_behavior(m_reinked_pane, m_reinked_scroll, image);
}