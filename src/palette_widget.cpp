#include "palette_widget.hpp"

#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFrame>
#include <QMenu>
#include <QContextMenuEvent>
#include <array>

using rgb_color = std::array<uint8_t, 3>;

namespace {

    class swatch : public QFrame {
        Q_OBJECT

    public:
        explicit swatch(const QColor& color, QWidget* parent = nullptr)
            : QFrame(parent), color_(color)
        {
            setFrameShape(QFrame::Box);
            setFrameShadow(QFrame::Plain);
            setLineWidth(1);
            setCursor(Qt::PointingHandCursor);
            setMinimumHeight(30);
        }

        QColor get_color() const { return color_; }
        void set_color(const QColor& c) { color_ = c; update(); }

    signals:
        void color_changed();
        void delete_requested();

    protected:
        void paintEvent(QPaintEvent* event) override {
            QFrame::paintEvent(event);
            QPainter painter(this);
            painter.fillRect(contentsRect(), color_);
        }

        void mousePressEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton) {
                QColor new_color = QColorDialog::getColor(color_, this, "Select Color");
                if (new_color.isValid()) {
                    color_ = new_color;
                    update();
                    emit color_changed();
                }
            }
            else {
                QFrame::mousePressEvent(event);
            }
        }

        void contextMenuEvent(QContextMenuEvent* event) override {
            QMenu menu(this);
            QAction* act_del = menu.addAction("Delete Swatch");
            connect(act_del, &QAction::triggered, this, &swatch::delete_requested);
            menu.exec(event->globalPos());
        }

    private:
        QColor color_;
    };

} // namespace

ser::palette_widget::palette_widget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    layout->addStretch();
}

// Right-click on the widget background to add colors
void ser::palette_widget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    QAction* add_act = menu.addAction("Add Color Swatch");

    if (menu.exec(event->globalPos()) == add_act) {
        QColor new_col = QColorDialog::getColor(Qt::white, this, "Add New Color");
        if (new_col.isValid()) {
            // Signal to main_window that we want a new color at the end
            emit color_added_requested(new_col);
        }
    }
}

void ser::palette_widget::insert_swatch(int index, const QColor& color) {
    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!layout) return;

    swatch* s = new swatch(color, this);

    connect(s, &swatch::color_changed, this, &ser::palette_widget::palette_changed);

    connect(s, &swatch::delete_requested, this, [this, s]() {
        auto it = std::find(swatches_.begin(), swatches_.end(), s);
        if (it != swatches_.end()) {
            int idx = static_cast<int>(std::distance(swatches_.begin(), it));
            emit color_delete_requested(idx);
        }
        });

    int layout_idx = std::min(index, layout->count() - 1);
    layout->insertWidget(layout_idx, s);

    if (index >= (int)swatches_.size()) swatches_.push_back(s);
    else swatches_.insert(swatches_.begin() + index, s);
}

void ser::palette_widget::remove_swatch_at(int index) {
    if (index < 0 || index >= (int)swatches_.size()) return;

    swatch* s = static_cast<swatch*>(swatches_[index]);
    swatches_.erase(swatches_.begin() + index);
    s->deleteLater();
    emit palette_changed();
}

void ser::palette_widget::set_colors(const std::vector<QColor>& colors) {
    clear_layout();
    for (const auto& c : colors) {
        insert_swatch(static_cast<int>(swatches_.size()), c);
    }
}

void ser::palette_widget::add_color(const QColor& color) {
    insert_swatch(static_cast<int>(swatches_.size()), color);
    emit palette_changed();
}

std::vector<QColor> ser::palette_widget::get_colors() const {
    std::vector<QColor> result;
    for (auto* s : swatches_) {
        result.push_back(static_cast<swatch*>(s)->get_color());
    }
    return result;
}

void ser::palette_widget::clear_layout() {
    for (QWidget* w : swatches_) w->deleteLater();
    swatches_.clear();
}

#include "palette_widget.moc"