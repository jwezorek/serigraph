#include "serigraph_widget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QDebug>

class ImagePane : public QWidget {
    Q_OBJECT
public:
    explicit ImagePane(QWidget* parent = nullptr) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    void set_image(const QImage& img) {
        m_image = img;
    }

    const QImage& image() const { return m_image; }

signals:
    void pixel_clicked(QColor color);

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);

        if (m_image.isNull()) {
            painter.fillRect(rect(), QColor(50, 50, 50)); 
            painter.setPen(Qt::lightGray);
            painter.drawText(rect(), Qt::AlignCenter, "[ No Image Loaded ]");
        } else {
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


ser::serigraph_widget::serigraph_widget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabPosition(QTabWidget::South);

    source_pane_ = create_tab("Source", source_scroll_);
    separated_pane_ = create_tab("Separated", separated_scroll_);
    reinked_pane_ = create_tab("Re-inked", reinked_scroll_);

    connect(source_pane_, &ImagePane::pixel_clicked,
        this, &serigraph_widget::source_pixel_clicked);
}

ImagePane* ser::serigraph_widget::create_tab(const QString& title, QScrollArea*& out_scroll_area) {

    out_scroll_area = new QScrollArea(this);
    out_scroll_area->setBackgroundRole(QPalette::Dark);
    out_scroll_area->setAlignment(Qt::AlignCenter);
    ImagePane* pane = new ImagePane(out_scroll_area);
    out_scroll_area->setWidget(pane);
    out_scroll_area->setWidgetResizable(true);
    addTab(out_scroll_area, title);

    return pane;
}

void ser::serigraph_widget::update_scroll_behavior(ImagePane* pane, QScrollArea* scroll, const QImage& img) {
    pane->set_image(img);

    if (img.isNull()) {
        scroll->setWidgetResizable(true);
    } else {
        scroll->setWidgetResizable(false);
        pane->resize(img.size());
    }
    pane->update();
}

void ser::serigraph_widget::set_source_image(const QImage& image) {
    update_scroll_behavior(source_pane_, source_scroll_, image);
    setCurrentWidget(source_scroll_); 
}

void ser::serigraph_widget::set_separated_image(const QImage& image) {
    update_scroll_behavior(separated_pane_, separated_scroll_, image);
}

void ser::serigraph_widget::set_reinked_image(const QImage& image) {
    update_scroll_behavior(reinked_pane_, reinked_scroll_, image);
}

QImage ser::serigraph_widget::src_image() const {
    return source_pane_->image();
}