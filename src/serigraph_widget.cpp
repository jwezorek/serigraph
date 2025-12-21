#include "serigraph_widget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QDebug>

namespace {

    class image_pane : public QWidget {

        Q_OBJECT

    public:
        explicit image_pane(QWidget* parent = nullptr) : QWidget(parent) {
            setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        }

        void set_image(const QImage& img) {
            image_ = img;
        }

        const QImage& image() const { return image_; }

    signals:
        void pixel_clicked(QColor color);

    protected:
        void paintEvent(QPaintEvent* event) override {
            QPainter painter(this);

            if (image_.isNull()) {
                painter.fillRect(rect(), QColor(50, 50, 50));
                painter.setPen(Qt::lightGray);
                painter.drawText(rect(), Qt::AlignCenter, "[ No Image Loaded ]");
            }
            else {
                painter.drawImage(0, 0, image_);
            }
        }

        void mousePressEvent(QMouseEvent* event) override {
            if (image_.isNull()) return;
            QPoint pos = event->pos();
            if (image_.valid(pos)) {
                emit pixel_clicked(image_.pixelColor(pos));
            }
        }

    private:
        QImage image_;
    };

    image_pane* create_tab(QTabWidget* parent, const QString& title, QScrollArea*& out_scroll_area) {

        out_scroll_area = new QScrollArea(parent);
        out_scroll_area->setBackgroundRole(QPalette::Dark);
        out_scroll_area->setAlignment(Qt::AlignCenter);
        image_pane* pane = new image_pane(out_scroll_area);
        out_scroll_area->setWidget(pane);
        out_scroll_area->setWidgetResizable(true);
        parent->addTab(out_scroll_area, title);

        return pane;
    }
}


#include "serigraph_widget.moc"


ser::serigraph_widget::serigraph_widget(QWidget* parent)
    : QTabWidget(parent)
{
    setTabPosition(QTabWidget::South);

    auto source_pane = create_tab(this, "Source", source_scroll_);
    source_pane_ = source_pane;
    separated_pane_ = create_tab(this, "Separated", separated_scroll_);
    reinked_pane_ = create_tab(this, "Re-inked", reinked_scroll_);

    connect(source_pane, &image_pane::pixel_clicked,
        this, &serigraph_widget::source_pixel_clicked);
}

void ser::serigraph_widget::update_scroll_behavior(QWidget* widget, QScrollArea* scroll, const QImage& img) {
    image_pane* pane = static_cast<image_pane*>(widget);
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
    return static_cast<image_pane*>(source_pane_)->image();
}