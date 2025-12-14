#include "palette_widget.hpp"

#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFrame>

// -------------------------------------------------------------------------
// Internal Helper: swatch (Anonymous Namespace)
// -------------------------------------------------------------------------
namespace {

    class swatch : public QFrame {
        Q_OBJECT

    public:
        explicit swatch(const QColor& color, int index, QWidget* parent = nullptr)
            : QFrame(parent), color_(color), index_(index)
        {
            setFrameShape(QFrame::Box);
            setFrameShadow(QFrame::Plain);
            setLineWidth(1);
            setCursor(Qt::PointingHandCursor);
            setMinimumHeight(30);
        }

        QColor get_color() const { return color_; }

    signals:
        void color_changed();

    protected:
        void paintEvent(QPaintEvent* event) override {
            QFrame::paintEvent(event);

            QPainter painter(this);
            QRect r = contentsRect();
            painter.fillRect(r, color_);
        }

        void mousePressEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton) {
                const QColor new_color = QColorDialog::getColor(
                    color_,
                    this,
                    "Select Ink Color",
                    QColorDialog::ShowAlphaChannel
                );

                if (new_color.isValid()) {
                    color_ = new_color;
                    update();
                    emit color_changed();
                }
            }
        }

    private:
        QColor color_;
        int index_;
    };

} // namespace


// -------------------------------------------------------------------------
// ser::palette_widget Implementation
// -------------------------------------------------------------------------

ser::palette_widget::palette_widget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addStretch();
}

void ser::palette_widget::set_colors(const std::vector<QColor>& colors) {
    clear_layout();

    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());

    // Remove the generic stretch
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item;
    }

    swatches_.clear();
    swatches_.reserve(colors.size());

    for (int i = 0; i < static_cast<int>(colors.size()); ++i) {
        // Instantiate the anonymous namespace class 'swatch'
        swatch* s = new swatch(colors[i], i, this);

        connect(s, &swatch::color_changed, this, &ser::palette_widget::palette_changed);

        layout->addWidget(s);
        swatches_.push_back(s);
    }

    layout->addStretch();
}

std::vector<QColor> ser::palette_widget::get_colors() const {
    std::vector<QColor> result;
    result.reserve(swatches_.size());

    for (const auto* frame : swatches_) {
        // Safe to static_cast because we only ever put 'swatch*' into this vector
        const auto* s = static_cast<const swatch*>(frame);
        result.push_back(s->get_color());
    }
    return result;
}

void ser::palette_widget::clear_layout() {
    QLayout* layout = this->layout();
    if (!layout) return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    swatches_.clear();
}

#include "palette_widget.moc"